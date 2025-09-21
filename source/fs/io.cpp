#include "fs/io.hpp"

#include "error.hpp"
#include "fs/SaveMetaData.hpp"
#include "fslib.hpp"
#include "logging/logger.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
#include "ui/PopMessageManager.hpp"

#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>

namespace
{
    // Size of buffer shared between threads.
    // constexpr size_t SIZE_FILE_BUFFER = 0x600000;
    // This one is just for testing something.
    constexpr size_t SIZE_FILE_BUFFER = 0x200000;
} // namespace

// clang-format off
struct FileThreadStruct : sys::threadpool::DataStruct
{
    std::mutex lock{};
    std::condition_variable condition{};
    bool bufferReady{};
    ssize_t readSize{};
    std::unique_ptr<sys::Byte[]> sharedBuffer{};
    fslib::File *source{};
};
// clang-format on

static void read_thread_function(sys::threadpool::JobData jobData)
{
    auto castData = std::static_pointer_cast<FileThreadStruct>(jobData);

    std::mutex &lock                           = castData->lock;
    std::condition_variable &condition         = castData->condition;
    bool &bufferReady                          = castData->bufferReady;
    ssize_t &readSize                          = castData->readSize;
    std::unique_ptr<sys::Byte[]> &sharedBuffer = castData->sharedBuffer;
    fslib::File &source                        = *castData->source;
    const int64_t fileSize                     = source.get_size();

    for (int64_t i = 0; i < fileSize;)
    {
        ssize_t localRead{};
        {
            std::unique_lock<std::mutex> bufferLock(lock);
            condition.wait(bufferLock, [&]() { return bufferReady == false; });

            readSize  = source.read(sharedBuffer.get(), SIZE_FILE_BUFFER);
            localRead = readSize;

            bufferReady = true;
            condition.notify_one();
        }
        if (localRead == -1) { break; }
        i += localRead;
    }
}

void fs::copy_file(const fslib::Path &source, const fslib::Path &destination, sys::ProgressTask *task)
{
    const char *statusTemplate = strings::get_by_name(strings::names::IO_STATUSES, 0);

    fslib::File sourceFile{source, FsOpenMode_Read};
    fslib::File destFile{destination, FsOpenMode_Create | FsOpenMode_Write, sourceFile.get_size()};
    if (error::fslib(sourceFile.is_open()) || error::fslib(destFile.is_open())) { return; }

    const int64_t sourceSize = sourceFile.get_size();
    if (task)
    {
        const std::string sourceString = source.string();
        std::string status             = stringutil::get_formatted_string(statusTemplate, sourceString.c_str());
        task->set_status(status);
        task->reset(static_cast<double>(sourceSize));
    }

    auto sharedData          = std::make_shared<FileThreadStruct>();
    sharedData->sharedBuffer = std::make_unique<sys::Byte[]>(SIZE_FILE_BUFFER);
    sharedData->source       = &sourceFile;

    auto localBuffer = std::make_unique<sys::Byte[]>(SIZE_FILE_BUFFER);

    std::mutex &lock                   = sharedData->lock;
    std::condition_variable &condition = sharedData->condition;
    bool &bufferReady                  = sharedData->bufferReady;
    ssize_t &readSize                  = sharedData->readSize;
    auto &sharedBuffer                 = sharedData->sharedBuffer;

    sys::threadpool::push_job(read_thread_function, sharedData);
    for (int64_t i = 0; i < sourceSize; i++)
    {
        ssize_t localRead{};
        {
            std::unique_lock<std::mutex> bufferLock(lock);
            condition.wait(bufferLock, [&]() { return bufferReady == true; });

            localRead = readSize;
            if (localRead == -1) { break; }

            std::memcpy(localBuffer.get(), sharedBuffer.get(), localRead);

            bufferReady = false;
            condition.notify_one();
        }
        // This should be checked. Not sure how yet...
        destFile.write(localBuffer.get(), localRead);

        i += localRead;
        if (task) { task->update_current(static_cast<double>(i)); }
    }
}

void fs::copy_file_commit(const fslib::Path &source,
                          const fslib::Path &destination,
                          int64_t journalSize,
                          sys::ProgressTask *task)
{
    const int popTicks                     = ui::PopMessageManager::DEFAULT_TICKS;
    const std::string_view popCommitFailed = strings::get_by_name(strings::names::IO_POPS, 0);
    const char *copyingStatus              = strings::get_by_name(strings::names::IO_STATUSES, 0);

    fslib::File sourceFile{source, FsOpenMode_Read};
    fslib::File destFile{destination, FsOpenMode_Create | FsOpenMode_Write, sourceFile.get_size()};
    if (error::fslib(sourceFile.is_open()) || error::fslib(destFile.is_open())) { return; }

    const int64_t sourceSize = sourceFile.get_size();
    if (task)
    {
        const std::string sourceString = source.string();
        std::string status             = stringutil::get_formatted_string(copyingStatus, sourceString.c_str());
        task->set_status(status);
        task->reset(static_cast<double>(sourceSize));
    }

    auto sharedData          = std::make_shared<FileThreadStruct>();
    sharedData->sharedBuffer = std::make_unique<sys::Byte[]>(SIZE_FILE_BUFFER);
    sharedData->source       = &sourceFile;

    auto localBuffer = std::make_unique<sys::Byte[]>(SIZE_FILE_BUFFER);

    std::mutex &lock                   = sharedData->lock;
    std::condition_variable &condition = sharedData->condition;
    bool &bufferReady                  = sharedData->bufferReady;
    ssize_t &readSize                  = sharedData->readSize;
    auto &sharedBuffer                 = sharedData->sharedBuffer;

    int64_t journalCount{};
    sys::threadpool::push_job(read_thread_function, sharedData);
    for (int64_t i = 0; i < sourceSize;)
    {
        ssize_t localRead{};
        {
            std::unique_lock<std::mutex> bufferlock{lock};
            condition.wait(bufferlock, [&]() { return bufferReady == true; });

            localRead = readSize;
            std::memcpy(localBuffer.get(), sharedBuffer.get(), localRead);

            bufferReady = false;
            condition.notify_one();
        }

        if (localRead == -1) { break; }

        const bool needsCommit = journalCount + localRead >= journalSize;
        if (needsCommit)
        {
            destFile.close();
            const bool commitError = error::fslib(fslib::commit_data_to_file_system(destination.get_device_name()));
            // To do: Handle this better. Threads current have no way of communicating errors.
            if (commitError) { ui::PopMessageManager::push_message(popTicks, popCommitFailed); }

            destFile.open(destination, FsOpenMode_Write);
            destFile.seek(i, destFile.BEGINNING);
        }

        destFile.write(localBuffer.get(), localRead);

        i += localRead;
        journalCount += localRead;
        if (task) { task->update_current(static_cast<double>(i)); }
    }
    destFile.close();

    const bool commitError = error::fslib(fslib::commit_data_to_file_system(destination.get_device_name()));
    if (commitError) { ui::PopMessageManager::push_message(popTicks, popCommitFailed); }
}

void fs::copy_directory(const fslib::Path &source, const fslib::Path &destination, sys::ProgressTask *task)
{
    fslib::Directory sourceDir{source};
    if (error::fslib(sourceDir.is_open())) { return; }

    for (const fslib::DirectoryEntry &entry : sourceDir)
    {
        const char *filename = entry.get_filename();
        if (filename == fs::NAME_SAVE_META) { continue; }

        const fslib::Path fullSource{source / filename};
        const fslib::Path fullDest{destination / filename};
        if (entry.is_directory())
        {
            const bool destExists  = fslib::directory_exists(fullDest);
            const bool createError = !destExists && error::fslib(fslib::create_directory(fullDest));
            if (!destExists && createError) { continue; }

            fs::copy_directory(fullSource, fullDest, task);
        }
        else { fs::copy_file(fullSource, fullDest, task); }
    }
}

void fs::copy_directory_commit(const fslib::Path &source,
                               const fslib::Path &destination,
                               int64_t journalSize,
                               sys::ProgressTask *task)
{
    fslib::Directory sourceDir{source};
    if (error::fslib(sourceDir.is_open())) { return; }

    for (const fslib::DirectoryEntry &entry : sourceDir)
    {
        const char *filename = entry.get_filename();
        if (filename == fs::NAME_SAVE_META) { continue; }

        const fslib::Path fullSource{source / filename};
        const fslib::Path fullDest{destination / filename};
        if (entry.is_directory())
        {
            const bool destExists  = fslib::directory_exists(fullDest);
            const bool createError = !destExists && error::fslib(fslib::create_directory(fullDest));
            const bool commitError =
                !createError && error::fslib(fslib::commit_data_to_file_system(fullDest.get_device_name()));
            if (!destExists && (createError || commitError)) { continue; }

            fs::copy_directory_commit(fullSource, fullDest, journalSize, task);
        }
        else { fs::copy_file_commit(fullSource, fullDest, journalSize, task); }
    }
}

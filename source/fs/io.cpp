#include "fs/io.hpp"

#include "error.hpp"
#include "fs/SaveMetaData.hpp"
#include "fslib.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "system/defines.hpp"
#include "ui/PopMessageManager.hpp"

#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>

namespace
{
    // Size of buffer shared between threads.
    // constexpr size_t FILE_BUFFER_SIZE = 0x600000;
    // This one is just for testing something.
    constexpr size_t SIZE_FILE_BUFFER = 0x200000;
} // namespace

// clang-format off
struct FileThreadStruct
{
    std::mutex lock{};
    std::condition_variable condition{};
    bool bufferReady{};
    ssize_t readSize{};
    std::unique_ptr<byte[]> sharedBuffer{};
};
// clang-format on

static void readThreadFunction(fslib::File &sourceFile, std::shared_ptr<FileThreadStruct> sharedData)
{
    std::mutex &lock                      = sharedData->lock;
    std::condition_variable &condition    = sharedData->condition;
    bool &bufferReady                     = sharedData->bufferReady;
    ssize_t &readSize                     = sharedData->readSize;
    std::unique_ptr<byte[]> &sharedBuffer = sharedData->sharedBuffer;
    const int64_t fileSize                = sourceFile.get_size();

    for (int64_t i = 0; i < fileSize;)
    {
        ssize_t localRead{};
        {
            std::unique_lock<std::mutex> bufferLock(lock);
            condition.wait(bufferLock, [&]() { return bufferReady == false; });

            readSize  = sourceFile.read(sharedBuffer.get(), SIZE_FILE_BUFFER);
            localRead = readSize;

            bufferReady = true;
            condition.notify_one();
        }
        if (localRead == -1) { break; }
        i += localRead;
    }
}

void fs::copy_file(const fslib::Path &source,
                   const fslib::Path &destination,
                   sys::ProgressTask *task,
                   uint64_t journalSize,
                   std::string_view commitDevice)
{
    const char *statusTemplate     = strings::get_by_name(strings::names::IO_STATUSES, 0);
    const char *popErrorCommitting = strings::get_by_name(strings::names::IO_POPS, 0);
    const int popticks             = ui::PopMessageManager::DEFAULT_TICKS;

    fslib::File sourceFile{source, FsOpenMode_Read};
    fslib::File destFile{destination, FsOpenMode_Create | FsOpenMode_Write, sourceFile.get_size()};
    if (error::fslib(sourceFile.is_open()) || error::fslib(destFile.is_open())) { return; }

    const bool needsCommits = journalSize > 0 && !commitDevice.empty();
    const int64_t fileSize  = sourceFile.get_size();
    if (task)
    {
        const std::string status = stringutil::get_formatted_string(statusTemplate, source.full_path());
        task->set_status(status);
        task->reset(static_cast<double>(fileSize));
    }

    auto sharedData          = std::make_shared<FileThreadStruct>();
    sharedData->sharedBuffer = std::make_unique<byte[]>(SIZE_FILE_BUFFER);
    auto localBuffer         = std::make_unique<byte[]>(SIZE_FILE_BUFFER);

    std::mutex &lock                      = sharedData->lock;
    std::condition_variable &condition    = sharedData->condition;
    bool &bufferReady                     = sharedData->bufferReady;
    ssize_t &readSize                     = sharedData->readSize;
    std::unique_ptr<byte[]> &sharedBuffer = sharedData->sharedBuffer;

    std::thread readThread(readThreadFunction, std::ref(sourceFile), sharedData);

    int64_t journalCount{};
    for (int64_t i = 0; i < fileSize; i++)
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

        const bool commitNeeded = needsCommits && (journalCount + localRead) >= static_cast<int64_t>(journalSize);
        if (commitNeeded)
        {
            destFile.close();
            const bool commitError = error::fslib(fslib::commit_data_to_file_system(commitDevice));
            if (commitError)
            {
                ui::PopMessageManager::push_message(popticks, popErrorCommitting);
                break;
            }

            destFile.open(destination, FsOpenMode_Write);
            destFile.seek(i, destFile.BEGINNING);

            journalCount = 0;
        }
        // This should be checked. Not sure how yet...
        destFile.write(localBuffer.get(), localRead);
        i += localRead;
        journalCount += localRead;
        if (task) { task->update_current(static_cast<double>(i)); }
    }

    readThread.join();
    destFile.close();

    const bool commitError = needsCommits && error::fslib(fslib::commit_data_to_file_system(commitDevice));
    if (commitError) { ui::PopMessageManager::push_message(popticks, popErrorCommitting); }
}

void fs::copy_directory(const fslib::Path &source,
                        const fslib::Path &destination,
                        sys::ProgressTask *task,
                        uint64_t journalSize,
                        std::string_view commitDevice)
{
    fslib::Directory sourceDir{source};
    if (error::fslib(sourceDir.is_open())) { return; }

    const int64_t dirCount = sourceDir.get_count();
    for (int64_t i = 0; i < dirCount; i++)
    {
        if (sourceDir[i] == fs::NAME_SAVE_META) { continue; }

        const fslib::Path fullSource{source / sourceDir[i]};
        const fslib::Path fullDest{destination / sourceDir[i]};
        if (sourceDir.is_directory(i))
        {
            const bool destExists  = fslib::directory_exists(fullDest);
            const bool createError = !destExists && fslib::create_directory(fullDest);
            if (!destExists && createError) { continue; }
            fs::copy_directory(fullSource, fullDest, task, journalSize, commitDevice);
        }
        else { fs::copy_file(fullSource, fullDest, task, journalSize, commitDevice); }
    }
}

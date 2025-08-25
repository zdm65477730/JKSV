#include "fs/zip.hpp"

#include "config/config.hpp"
#include "fs/SaveMetaData.hpp"
#include "logging/error.hpp"
#include "logging/logger.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
#include "ui/PopMessageManager.hpp"

#include <condition_variable>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>
#include <thread>

namespace
{
    /// @brief Buffer size used for writing files to ZIP.
    constexpr size_t SIZE_ZIP_BUFFER = 0x100000;

    /// @brief Buffer size used for decompressing files from ZIP.
    constexpr size_t SIZE_UNZIP_BUFFER = 0x600000;
} // namespace

// Shared struct for Zip/File IO
// clang-format off
struct ZipIOStruct
{
    std::mutex lock{};
    std::condition_variable condition{};
    ssize_t readSize{};
    bool bufferReady{};
    std::unique_ptr<sys::byte[]> sharedBuffer{};
};
// clang-format on

// Function for reading files for Zipping.
static void zipReadThreadFunction(fslib::File &source, std::shared_ptr<ZipIOStruct> sharedData)
{
    std::mutex &lock                           = sharedData->lock;
    std::condition_variable &condition         = sharedData->condition;
    ssize_t &readSize                          = sharedData->readSize;
    bool &bufferReady                          = sharedData->bufferReady;
    std::unique_ptr<sys::byte[]> &sharedBuffer = sharedData->sharedBuffer;
    const int64_t fileSize                     = source.get_size();

    for (int64_t i = 0; i < fileSize;)
    {
        ssize_t localRead{}; // This is a local variable to store the read size so we don't need to hold the other thread up.
        {
            std::unique_lock<std::mutex> bufferLock(lock);
            condition.wait(bufferLock, [&]() { return bufferReady == false; });

            readSize  = source.read(sharedBuffer.get(), SIZE_ZIP_BUFFER);
            localRead = readSize;

            bufferReady = true;
            condition.notify_one();
        }
        if (readSize == -1) { break; }
        i += localRead;
    }
}

// Function for reading data from Zip to buffer.
static void unzipReadThreadFunction(fs::MiniUnzip &unzip, std::shared_ptr<ZipIOStruct> sharedData)
{
    std::mutex &lock                           = sharedData->lock;
    std::condition_variable &condition         = sharedData->condition;
    ssize_t &readSize                          = sharedData->readSize;
    bool &bufferReady                          = sharedData->bufferReady;
    std::unique_ptr<sys::byte[]> &sharedBuffer = sharedData->sharedBuffer;
    const int64_t fileSize                     = unzip.get_uncompressed_size();

    for (int64_t i = 0; i < fileSize;)
    {
        ssize_t localRead{};
        {
            std::unique_lock<std::mutex> bufferLock(lock);
            condition.wait(bufferLock, [&]() { return bufferReady == false; });

            readSize  = unzip.read(sharedBuffer.get(), SIZE_UNZIP_BUFFER);
            localRead = readSize;

            bufferReady = true;
            condition.notify_one();
        }
        if (localRead == -1) { break; }
        i += localRead;
    }
}

void fs::copy_directory_to_zip(const fslib::Path &source, fs::MiniZip &dest, sys::ProgressTask *task)
{
    const char *ioStatus = strings::get_by_name(strings::names::IO_STATUSES, 1);

    fslib::Directory sourceDir{source};
    if (error::fslib(sourceDir.is_open())) { return; }

    for (const fslib::DirectoryEntry &entry : sourceDir.list())
    {
        const fslib::Path fullSource{source / entry};
        if (entry.is_directory()) { fs::copy_directory_to_zip(fullSource, dest, task); }
        else
        {
            fslib::File sourceFile{fullSource, FsOpenMode_Read};
            const bool newZipFile = dest.open_new_file(fullSource.full_path());
            if (error::fslib(sourceFile.is_open()) || !newZipFile) { continue; }

            const int64_t fileSize   = sourceFile.get_size();
            auto sharedData          = std::make_shared<ZipIOStruct>();
            sharedData->sharedBuffer = std::make_unique<sys::byte[]>(SIZE_ZIP_BUFFER);
            auto localBuffer         = std::make_unique<sys::byte[]>(SIZE_ZIP_BUFFER);

            if (task)
            {
                const std::string status = stringutil::get_formatted_string(ioStatus, fullSource.full_path());
                task->set_status(status);
                task->reset(static_cast<double>(fileSize));
            }

            // I just like doing this. It makes things easier to type.
            std::mutex &lock                   = sharedData->lock;
            std::condition_variable &condition = sharedData->condition;
            ssize_t &readSize                  = sharedData->readSize;
            bool &bufferReady                  = sharedData->bufferReady;
            auto &sharedBuffer                 = sharedData->sharedBuffer;

            std::thread readThread(zipReadThreadFunction, std::ref(sourceFile), sharedData);
            for (int64_t i = 0; i < fileSize;)
            {
                ssize_t localRead{};
                {
                    std::unique_lock<std::mutex> bufferLock(lock);
                    condition.wait(bufferLock, [&]() { return bufferReady == true; });

                    localRead = readSize;

                    std::memcpy(localBuffer.get(), sharedBuffer.get(), localRead);

                    bufferReady = false;
                    condition.notify_one();
                }
                const bool writeGood = localRead != -1 && dest.write(localBuffer.get(), localRead);
                if (!writeGood) { break; }

                i += localRead;

                if (task) { task->update_current(static_cast<double>(i)); }
            }

            dest.close_current_file();
            readThread.join();
        }
    }
}

void fs::copy_zip_to_directory(fs::MiniUnzip &unzip,
                               const fslib::Path &dest,
                               int64_t journalSize,
                               std::string_view commitDevice,
                               sys::ProgressTask *task)
{
    if (!unzip.reset()) { return; }
    const int popTicks          = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popCommitFailed = strings::get_by_name(strings::names::IO_POPS, 0);
    const char *statusTemplate  = strings::get_by_name(strings::names::IO_STATUSES, 2);
    const bool needCommits      = journalSize > 0 && !commitDevice.empty();

    do {
        if (unzip.get_filename() == fs::NAME_SAVE_META) { continue; }

        fslib::Path fullDest{dest / unzip.get_filename()};
        const size_t lastDir = fullDest.find_last_of('/');
        if (lastDir == fullDest.NOT_FOUND) { continue; }

        const fslib::Path dirPath{fullDest.sub_path(lastDir)};
        const bool exists      = dirPath.is_valid() && fslib::directory_exists(dirPath);
        const bool createError = dirPath.is_valid() && !exists && error::fslib(fslib::create_directories_recursively(dirPath));
        if (dirPath.is_valid() && !exists && createError) { continue; }

        const int64_t fileSize = unzip.get_uncompressed_size();
        fslib::File destFile{fullDest, FsOpenMode_Create | FsOpenMode_Write, fileSize};
        if (error::fslib(destFile.is_open())) { continue; }

        if (task)
        {
            const std::string status = stringutil::get_formatted_string(statusTemplate, unzip.get_filename());
            task->set_status(status);
            task->reset(static_cast<double>(fileSize));
        }

        auto sharedData          = std::make_shared<ZipIOStruct>();
        sharedData->sharedBuffer = std::make_unique<sys::byte[]>(SIZE_UNZIP_BUFFER);
        auto localBuffer         = std::make_unique<sys::byte[]>(SIZE_UNZIP_BUFFER);

        std::mutex &lock                           = sharedData->lock;
        std::condition_variable &condition         = sharedData->condition;
        ssize_t &readSize                          = sharedData->readSize;
        bool &bufferReady                          = sharedData->bufferReady;
        std::unique_ptr<sys::byte[]> &sharedBuffer = sharedData->sharedBuffer;

        std::thread readThread(unzipReadThreadFunction, std::ref(unzip), sharedData);
        int64_t journalCount{};
        for (int64_t i = 0; i < fileSize;)
        {
            ssize_t localRead{};
            {
                std::unique_lock<std::mutex> bufferLock(lock);
                condition.wait(bufferLock, [&]() { return bufferReady == true; });

                localRead = readSize;
                std::memcpy(localBuffer.get(), sharedBuffer.get(), localRead);

                bufferReady = false;
                condition.notify_one();
            }

            const bool commitNeeded = needCommits && journalCount + localRead >= journalSize;
            if (commitNeeded)
            {
                destFile.close();
                const bool commitError = error::fslib(fslib::commit_data_to_file_system(commitDevice));
                if (commitError) { ui::PopMessageManager::push_message(popTicks, popCommitFailed); } // To do: How to recover?

                destFile.open(fullDest, FsOpenMode_Write);
                destFile.seek(i, destFile.BEGINNING);
                journalCount = 0;
            }
            const bool goodWrite = localRead != -1 && destFile.write(localBuffer.get(), localRead);
            error::fslib(goodWrite); // This will catch the error. To do: Recovery.

            i += localRead;
            journalCount += localRead;
            if (task) { task->update_current(static_cast<double>(i)); }
        }
        readThread.join();
        destFile.close();

        const bool commitError = needCommits && error::fslib(fslib::commit_data_to_file_system(commitDevice));
        if (commitError) { ui::PopMessageManager::push_message(popTicks, popCommitFailed); }
    } while (unzip.next_file());
}

bool fs::zip_has_contents(const fslib::Path &zipPath)
{
    fs::MiniUnzip unzip{zipPath};
    if (!unzip.is_open()) { return false; }

    do {
        if (unzip.get_filename() != fs::NAME_SAVE_META) { return true; }
    } while (unzip.next_file());
    return false;
}

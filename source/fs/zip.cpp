#include "fs/zip.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include <condition_variable>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>
#include <thread>

namespace
{
    // Size used for Zipping files.
    constexpr size_t ZIP_BUFFER_SIZE = 0x100000;
    // Size used for unzipping.
    constexpr size_t UNZIP_BUFFER_SIZE = 0x600000;
} // namespace

// Shared struct for Zip/File IO
typedef struct
{
        // Mutex and condition for buffer.
        std::mutex m_bufferLock;
        std::condition_variable m_bufferCondition;
        bool m_bufferIsFull = false;
        // Number of bytes read from file.
        ssize_t m_readCount = 0;
        // Shared/reading buffer.
        std::unique_ptr<unsigned char[]> m_sharedBuffer;
} ZipIOStruct;

// Function for reading files for Zipping.
static void zipReadThreadFunction(fslib::File &source, std::shared_ptr<ZipIOStruct> sharedData)
{
    int64_t fileSize = source.getSize();
    for (int64_t readCount = 0; readCount < fileSize;)
    {
        // Read into shared buffer.
        sharedData->m_readCount = source.read(sharedData->m_sharedBuffer.get(), ZIP_BUFFER_SIZE);
        // Update read count
        readCount += sharedData->m_readCount;
        // Signal other thread buffer is ready to go.
        sharedData->m_bufferIsFull = true;
        sharedData->m_bufferCondition.notify_one();
        // Wait for other thread to release lock on buffer so this thread can read again.
        std::unique_lock<std::mutex> m_bufferLock(sharedData->m_bufferLock);
        sharedData->m_bufferCondition.wait(m_bufferLock, [&sharedData]() { return sharedData->m_bufferIsFull == false; });
    }
}

// Function for reading data from Zip to buffer.
static void unzipReadThreadFunction(unzFile source, int64_t fileSize, std::shared_ptr<ZipIOStruct> sharedData)
{
    for (int64_t readCount = 0; readCount < fileSize;)
    {
        // Read from zip file.
        sharedData->m_readCount = unzReadCurrentFile(source, sharedData->m_sharedBuffer.get(), UNZIP_BUFFER_SIZE);

        readCount += sharedData->m_readCount;

        sharedData->m_bufferIsFull = true;
        sharedData->m_bufferCondition.notify_one();

        std::unique_lock<std::mutex> m_bufferLock(sharedData->m_bufferLock);
        sharedData->m_bufferCondition.wait(m_bufferLock, [&sharedData]() { return sharedData->m_bufferIsFull == false; });
    }
}

void fs::copyDirectoryToZip(const fslib::Path &source, zipFile destination, sys::ProgressTask *task)
{
    fslib::Directory sourceDir(source);
    if (!sourceDir.isOpen())
    {
        logger::log("Error opening source directory: %s", fslib::getErrorString());
        return;
    }

    for (int64_t i = 0; i < sourceDir.getCount(); i++)
    {
        if (sourceDir.isDirectory(i))
        {
            fslib::Path newSource = source / sourceDir[i];
            fs::copyDirectoryToZip(newSource, destination, task);
        }
        else
        {
            // Open source file.
            fslib::Path fullSource = source / sourceDir[i];
            fslib::File sourceFile(fullSource, FsOpenMode_Read);
            if (!sourceFile.isOpen())
            {
                logger::log("Error zipping file: %s", fslib::getErrorString());
                continue;
            }

            // Date for file(s)
            std::time_t timer;
            std::time(&timer);
            std::tm *localTime = std::localtime(&timer);
            zip_fileinfo FileInfo = {.tmz_date = {.tm_sec = localTime->tm_sec,
                                                  .tm_min = localTime->tm_min,
                                                  .tm_hour = localTime->tm_hour,
                                                  .tm_mday = localTime->tm_mday,
                                                  .tm_mon = localTime->tm_mon,
                                                  .tm_year = localTime->tm_year + 1900},
                                     .dosDate = 0,
                                     .internal_fa = 0,
                                     .external_fa = 0};

            // Create new file in zip
            const char *zipNameBegin = std::strchr(fullSource.getPath(), '/') + 1;
            int zipError = zipOpenNewFileInZip64(destination,
                                                 zipNameBegin,
                                                 &FileInfo,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 Z_DEFLATED,
                                                 config::getByKey(config::keys::ZIP_COMPRESSION_LEVEL),
                                                 1);
            if (zipError != ZIP_OK)
            {
                logger::log("Error creating file in zip: %i.", zipError);
                continue;
            }

            // Shared data for thread.
            std::shared_ptr<ZipIOStruct> sharedData(new ZipIOStruct);
            sharedData->m_sharedBuffer = std::make_unique<unsigned char[]>(ZIP_BUFFER_SIZE);

            // Local buffer for writing.
            std::unique_ptr<unsigned char[]> localBuffer(new unsigned char[ZIP_BUFFER_SIZE]);

            // Update task if passed.
            if (task)
            {
                task->setStatus(strings::getByName(strings::names::COPYING_FILES, 1), fullSource.cString());
                task->reset(static_cast<double>(sourceFile.getSize()));
            }

            std::thread readThread(zipReadThreadFunction, std::ref(sourceFile), sharedData);

            int64_t fileSize = sourceFile.getSize();
            for (int64_t writeCount = 0, readCount = 0; writeCount < fileSize;)
            {
                {
                    // Wait for buffer signal
                    std::unique_lock<std::mutex> m_bufferLock(sharedData->m_bufferLock);
                    sharedData->m_bufferCondition.wait(m_bufferLock, [&sharedData]() { return sharedData->m_bufferIsFull; });

                    // Save read count, copy shared to local.
                    readCount = sharedData->m_readCount;
                    std::memcpy(localBuffer.get(), sharedData->m_sharedBuffer.get(), readCount);

                    // Signal copy was good and release lock.
                    sharedData->m_bufferIsFull = false;
                    sharedData->m_bufferCondition.notify_one();
                }
                // Write
                zipError = zipWriteInFileInZip(destination, localBuffer.get(), readCount);
                if (zipError != ZIP_OK)
                {
                    logger::log("Error writing data to zip: %i.", zipError);
                }
                // Update count and status
                writeCount += readCount;
                if (task)
                {
                    task->updateCurrent(static_cast<double>(writeCount));
                }
            }
            // Wait for thread
            readThread.join();
            // Close file in zip
            zipCloseFileInZip(destination);
        }
    }
}

void fs::copyZipToDirectory(unzFile source,
                            const fslib::Path &destination,
                            uint64_t journalSize,
                            std::string_view commitDevice,
                            sys::ProgressTask *task)
{
    int zipError = unzGoToFirstFile(source);
    if (zipError != UNZ_OK)
    {
        logger::log("Error opening empty ZIP file: %i.", zipError);
        return;
    }

    do
    {
        // Get file information.
        unz_file_info64 currentFileInfo;
        char filename[FS_MAX_PATH] = {0};
        if (unzGetCurrentFileInfo64(source, &currentFileInfo, filename, FS_MAX_PATH, NULL, 0, NULL, 0) != UNZ_OK ||
            unzOpenCurrentFile(source) != UNZ_OK)
        {
            logger::log("Error opening and getting information for file in zip.");
            continue;
        }

        // Create full path to item, make sure directories are created if needed.
        fslib::Path fullDestination = destination / filename;

        fslib::Path directories = fullDestination.subPath(fullDestination.findLastOf('/') - 1);
        // To do: Make FsLib handle this correctly. First condition is a workaround for now...
        if (directories.isValid() && !fslib::createDirectoriesRecursively(directories))
        {
            logger::log("Error creating zip file path \"%s\": %s", directories.cString(), fslib::getErrorString());
            continue;
        }

        fslib::File destinationFile(fullDestination, FsOpenMode_Create | FsOpenMode_Write, currentFileInfo.uncompressed_size);
        if (!destinationFile.isOpen())
        {
            logger::log("Error creating file from zip: %s", fslib::getErrorString());
            continue;
        }

        // Shared data for both threads
        std::shared_ptr<ZipIOStruct> sharedData(new ZipIOStruct);
        sharedData->m_sharedBuffer = std::make_unique<unsigned char[]>(UNZIP_BUFFER_SIZE);

        // Spawn read thread.
        std::thread readThread(unzipReadThreadFunction, source, currentFileInfo.uncompressed_size, sharedData);

        // Local buffer
        std::unique_ptr<unsigned char[]> localBuffer(new unsigned char[UNZIP_BUFFER_SIZE]);

        // Set status
        if (task)
        {
            task->setStatus(strings::getByName(strings::names::COPYING_FILES, 3), filename);
            task->reset(static_cast<double>(currentFileInfo.uncompressed_size));
        }

        for (int64_t writeCount = 0, readCount = 0, journalCount = 0; writeCount < static_cast<int64_t>(currentFileInfo.uncompressed_size);)
        {
            {
                // Wait for buffer.
                std::unique_lock<std::mutex> m_bufferLock(sharedData->m_bufferLock);
                sharedData->m_bufferCondition.wait(m_bufferLock, [&sharedData]() { return sharedData->m_bufferIsFull; });

                // Save read count for later
                readCount = sharedData->m_readCount;

                // Copy shared to local
                std::memcpy(localBuffer.get(), sharedData->m_sharedBuffer.get(), readCount);

                // Signal this thread is done.
                sharedData->m_bufferIsFull = false;
                sharedData->m_bufferCondition.notify_one();
            }

            // Journaling check
            if (journalCount + readCount >= static_cast<int64_t>(journalSize))
            {
                // Close.
                destinationFile.close();
                // Commit
                if (!fslib::commitDataToFileSystem(commitDevice))
                {
                    logger::log("Error committing data to save: %s", fslib::getErrorString());
                }
                // Reopen, seek to previous position.
                destinationFile.open(fullDestination, FsOpenMode_Write);
                destinationFile.seek(writeCount, destinationFile.beginning);
                // Reset journal
                journalCount = 0;
            }
            // Write data.
            destinationFile.write(localBuffer.get(), readCount);
            // Update write and journal count
            writeCount += readCount;
            journalCount += journalCount;
            // Update status
            if (task)
            {
                task->updateCurrent(writeCount);
            }
        }
        // Close file and commit again just for good measure.
        destinationFile.close();
        if (!fslib::commitDataToFileSystem(commitDevice))
        {
            logger::log("Error performing final file commit: %s", fslib::getErrorString());
        }
    } while (unzGoToNextFile(source) != UNZ_END_OF_LIST_OF_FILE);
}

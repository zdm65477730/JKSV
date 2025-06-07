#include "fs/zip.hpp"
#include "config.hpp"
#include "fs/SaveMetaData.hpp"
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
    /// @brief Buffer size used for writing files to ZIP.
    constexpr size_t SIZE_ZIP_BUFFER = 0x100000;

    /// @brief Buffer size used for decompressing files from ZIP.
    constexpr size_t SIZE_UNZIP_BUFFER = 0x600000;
} // namespace

// Shared struct for Zip/File IO
typedef struct
{
        /// @brief Mutex for blocking the shared buffer.
        std::mutex m_bufferLock;

        /// @brief Conditional for locking and unlocking.
        std::condition_variable m_bufferCondition;

        /// @brief Bool that lets threads communicate when they are using the buffer.
        bool m_bufferIsFull = false;

        /// @brief Number of bytes read from the file.
        ssize_t m_readCount = 0;

        /// @brief Shared/reading buffer.
        std::unique_ptr<unsigned char[]> m_sharedBuffer;
} ZipIOStruct;

// Function for reading files for Zipping.
static void zipReadThreadFunction(fslib::File &source, std::shared_ptr<ZipIOStruct> sharedData)
{
    // Don't call this every loop. Not sure if compiler optimizes that out or not now.
    int64_t fileSize = source.get_size();

    // Loop until the file is completely read.
    for (int64_t readCount = 0; readCount < fileSize;)
    {
        // Read into shared buffer.
        sharedData->m_readCount = source.read(sharedData->m_sharedBuffer.get(), SIZE_ZIP_BUFFER);

        // Update read count
        readCount += sharedData->m_readCount;

        // Signal other thread buffer is ready to go.
        sharedData->m_bufferIsFull = true;
        sharedData->m_bufferCondition.notify_one();

        // Wait for other thread to release lock on buffer so this thread can read again.
        std::unique_lock<std::mutex> bufferLock(sharedData->m_bufferLock);
        sharedData->m_bufferCondition.wait(bufferLock, [&sharedData]() { return sharedData->m_bufferIsFull == false; });
    }
}

// Function for reading data from Zip to buffer.
static void unzipReadThreadFunction(unzFile source, int64_t fileSize, std::shared_ptr<ZipIOStruct> sharedData)
{
    for (int64_t readCount = 0; readCount < fileSize;)
    {
        // Read from zip file.
        sharedData->m_readCount = unzReadCurrentFile(source, sharedData->m_sharedBuffer.get(), SIZE_UNZIP_BUFFER);

        readCount += sharedData->m_readCount;

        sharedData->m_bufferIsFull = true;
        sharedData->m_bufferCondition.notify_one();

        std::unique_lock<std::mutex> bufferLock(sharedData->m_bufferLock);
        sharedData->m_bufferCondition.wait(bufferLock, [&sharedData]() { return sharedData->m_bufferIsFull == false; });
    }
}

void fs::copy_directory_to_zip(const fslib::Path &source, zipFile destination, sys::ProgressTask *task)
{
    fslib::Directory sourceDir(source);
    if (!sourceDir)
    {
        logger::log("Error opening source directory: %s", fslib::get_error_string());
        return;
    }

    // Grab this here instead of calling the config function for every file.
    int compressionLevel = config::get_by_key(config::keys::ZIP_COMPRESSION_LEVEL);

    for (int64_t i = 0; i < sourceDir.get_count(); i++)
    {
        if (sourceDir.is_directory(i))
        {
            fslib::Path newSource = source / sourceDir[i];
            fs::copy_directory_to_zip(newSource, destination, task);
        }
        else
        {
            // Open source file.
            fslib::Path fullSource = source / sourceDir[i];
            fslib::File sourceFile(fullSource, FsOpenMode_Read);
            if (!sourceFile)
            {
                logger::log("Error zipping file: %s", fslib::get_error_string());
                continue;
            }

            // Zip info
            zip_fileinfo fileInfo;
            fs::create_zip_fileinfo(fileInfo);

            // Create new file in zip
            const char *zipNameBegin = std::strchr(fullSource.get_path(), '/') + 1;
            int zipError = zipOpenNewFileInZip64(destination,
                                                 zipNameBegin,
                                                 &fileInfo,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 Z_DEFLATED,
                                                 compressionLevel,
                                                 0);
            if (zipError != ZIP_OK)
            {
                logger::log("Error creating file in zip: %i.", zipError);
                continue;
            }

            // Shared data for thread.
            std::shared_ptr<ZipIOStruct> sharedData(new ZipIOStruct);
            sharedData->m_sharedBuffer = std::make_unique<unsigned char[]>(SIZE_ZIP_BUFFER);

            // Local buffer for writing.
            std::unique_ptr<unsigned char[]> localBuffer(new unsigned char[SIZE_ZIP_BUFFER]);

            // Update task if passed.
            if (task)
            {
                task->set_status(strings::get_by_name(strings::names::COPYING_FILES, 1), fullSource.c_string());
                task->reset(static_cast<double>(sourceFile.get_size()));
            }

            // To do: Thread pool to avoid spawning threads like this.
            std::thread readThread(zipReadThreadFunction, std::ref(sourceFile), sharedData);

            int64_t fileSize = sourceFile.get_size();
            for (int64_t writeCount = 0, readCount = 0; writeCount < fileSize;)
            {
                {
                    // Wait for buffer signal
                    std::unique_lock<std::mutex> m_bufferLock(sharedData->m_bufferLock);
                    sharedData->m_bufferCondition.wait(m_bufferLock,
                                                       [&sharedData]() { return sharedData->m_bufferIsFull; });

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
                    task->update_current(static_cast<double>(writeCount));
                }
            }
            // Wait for thread
            readThread.join();
            // Close file in zip
            zipCloseFileInZip(destination);
        }
    }
}

void fs::copy_zip_to_directory(unzFile source,
                               const fslib::Path &destination,
                               uint64_t journalSize,
                               std::string_view commitDevice,
                               sys::ProgressTask *task)
{
    // With the new save meta, this might never fail... Need to figure this out some time.
    int zipError = unzGoToFirstFile(source);
    if (zipError != UNZ_OK)
    {
        logger::log("Error unzipping file: Zip is empty!");
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
            logger::log("Error getting information for or opening file for reading in zip!");
            continue;
        }

        // Save meta file filter.
        if (filename == fs::NAME_SAVE_META)
        {
            continue;
        }

        // Create full path to item, make sure directories are created if needed.
        fslib::Path fullDestination = destination / filename;

        fslib::Path directories = fullDestination.sub_path(fullDestination.find_last_of('/'));

        // To do: Make FsLib handle this correctly. First condition is a workaround for now...
        if (directories.is_valid() && !fslib::create_directories_recursively(directories))
        {
            logger::log("Error creating zip file path \"%s\": %s", directories.c_string(), fslib::get_error_string());
            continue;
        }

        fslib::File destinationFile(fullDestination,
                                    FsOpenMode_Create | FsOpenMode_Write,
                                    currentFileInfo.uncompressed_size);
        if (!destinationFile)
        {
            logger::log("Error creating file from zip: %s", fslib::get_error_string());
            continue;
        }

        // Shared data for both threads
        std::shared_ptr<ZipIOStruct> sharedData(new ZipIOStruct);
        sharedData->m_sharedBuffer = std::make_unique<unsigned char[]>(SIZE_UNZIP_BUFFER);

        // Local buffer
        std::unique_ptr<unsigned char[]> localBuffer(new unsigned char[SIZE_UNZIP_BUFFER]);

        // Spawn read thread.
        std::thread readThread(unzipReadThreadFunction, source, currentFileInfo.uncompressed_size, sharedData);

        // Set status
        if (task)
        {
            task->set_status(strings::get_by_name(strings::names::COPYING_FILES, 2), filename);
            task->reset(static_cast<double>(currentFileInfo.uncompressed_size));
        }

        for (int64_t writeCount = 0, readCount = 0, journalCount = 0;
             writeCount < static_cast<int64_t>(currentFileInfo.uncompressed_size);)
        {
            {
                // Wait for buffer.
                std::unique_lock<std::mutex> bufferLock(sharedData->m_bufferLock);
                sharedData->m_bufferCondition.wait(bufferLock, [&sharedData]() { return sharedData->m_bufferIsFull; });

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
                if (!fslib::commit_data_to_file_system(commitDevice))
                {
                    logger::log("Error committing data to save: %s", fslib::get_error_string());
                }

                // Reopen, seek to previous position.
                destinationFile.open(fullDestination, FsOpenMode_Write);
                destinationFile.seek(writeCount, destinationFile.BEGINNING);

                // Reset journal
                journalCount = 0;
            }
            // Write data.
            destinationFile.write(localBuffer.get(), readCount);

            // Update write and journal count
            writeCount += readCount;
            journalCount += readCount;

            // Update status
            if (task)
            {
                task->update_current(writeCount);
            }
        }

        // Join the read thread.
        readThread.join();

        // Close file and commit again just for good measure.
        destinationFile.close();
        if (!fslib::commit_data_to_file_system(commitDevice))
        {
            logger::log("Error performing final file commit: %s", fslib::get_error_string());
        }
    } while (unzGoToNextFile(source) != UNZ_END_OF_LIST_OF_FILE);
}

void fs::create_zip_fileinfo(zip_fileinfo &info)
{
    // Grab the current time.
    std::time_t currentTime = std::time(NULL);

    // Get the local time.
    std::tm *localTime = std::localtime(&currentTime);

    // Create struct to return.
    info = {.tmz_date = {.tm_sec = localTime->tm_sec,
                         .tm_min = localTime->tm_min,
                         .tm_hour = localTime->tm_hour,
                         .tm_mday = localTime->tm_mday,
                         .tm_mon = localTime->tm_mon,
                         .tm_year = localTime->tm_year + 1900},
            .dosDate = 0,
            .internal_fa = 0,
            .external_fa = 0};
}

bool fs::zip_has_contents(const fslib::Path &zipPath)
{
    unzFile testZip = unzOpen(zipPath.c_string());
    if (!testZip)
    {
        return false;
    }

    int zipError = unzGoToFirstFile(testZip);
    if (zipError != UNZ_OK)
    {
        unzClose(testZip);
        return false;
    }
    unzClose(testZip);
    return true;
}

bool fs::locate_file_in_zip(unzFile zip, std::string_view name)
{
    // Go to the first file, first.
    int zipError = unzGoToFirstFile(zip);
    if (zipError != UNZ_OK)
    {
        logger::log("Error locating file: Zip is empty!");
        return false;
    }

    // This should be a large enough buffer.
    char filename[FS_MAX_PATH] = {0};
    // File info.
    unz_file_info64 fileinfo = {0};

    // Loop through files. If minizip has a better way of doing this, I couldn't find it.
    do
    {
        // Grab this stuff.
        zipError = unzGetCurrentFileInfo64(zip, &fileinfo, filename, FS_MAX_PATH, NULL, 0, NULL, 0);
        if (zipError != UNZ_OK)
        {
            continue;
        }
        else if (filename == name)
        {
            return true;
        }
    } while (unzGoToNextFile(zip) != UNZ_END_OF_LIST_OF_FILE);

    // Guess it wasn't found?
    return false;
}

uint64_t fs::get_zip_total_size(unzFile zip)
{
    // First, first.
    int zipError = unzGoToFirstFile(zip);
    if (zipError != UNZ_OK)
    {
        logger::log("Error getting total zip file size: %i.", zipError);
        return 0;
    }

    // Size.
    uint64_t zipSize = 0;

    // File's name and info buffers.
    char filename[FS_MAX_PATH] = {0};
    unz_file_info64 fileinfo = {0};

    do
    {
        zipError = unzGetCurrentFileInfo64(zip, &fileinfo, filename, 0, NULL, 0, NULL, 0);
        if (zipError != UNZ_OK)
        {
            continue;
        }

        // Add
        zipSize += fileinfo.uncompressed_size;
    } while (unzGoToNextFile(zip) != UNZ_END_OF_LIST_OF_FILE);

    // Reset. Maybe this should be error checked, but I don't see the point here?
    unzGoToFirstFile(zip);

    return zipSize;
}

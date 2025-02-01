#include "fs/io.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>

namespace
{
    // Size of buffer shared between threads.
    // constexpr size_t FILE_BUFFER_SIZE = 0x600000;
    // This one is just for testing something.
    constexpr size_t FILE_BUFFER_SIZE = 0x80000;
} // namespace

// Struct threads shared to read and write files.
typedef struct
{
        // Mutex to lock buffer.
        std::mutex m_bufferLock;
        // Conditional to wait on signals.
        std::condition_variable m_bufferCondition;
        // Bool to control signals.
        bool m_bufferIsFull = false;
        // Number of bytes read.
        size_t m_readSize = 0;
        // Shared (read) buffer.
        std::unique_ptr<unsigned char[]> m_readBuffer;
} FileTransferStruct;

// This function Reads into the buffer. The other thread writes.
static void readThreadFunction(fslib::File &sourceFile, std::shared_ptr<FileTransferStruct> sharedData)
{
    int64_t fileSize = sourceFile.getSize();
    for (int64_t readCount = 0; readCount < fileSize;)
    {
        // Read data to shared buffer.
        sharedData->m_readSize = sourceFile.read(sharedData->m_readBuffer.get(), FILE_BUFFER_SIZE);
        // Update local read count
        readCount += sharedData->m_readSize;
        // Signal to other thread buffer is full.
        sharedData->m_bufferIsFull = true;
        sharedData->m_bufferCondition.notify_one();
        // Wait for other thread to signal buffer is empty. Lock is released immediately, but it works and that's what matters.
        std::unique_lock<std::mutex> m_bufferLock(sharedData->m_bufferLock);
        sharedData->m_bufferCondition.wait(m_bufferLock, [&sharedData]() { return sharedData->m_bufferIsFull == false; });
    }
}

void fs::copyFile(const fslib::Path &source,
                  const fslib::Path &destination,
                  uint64_t journalSize,
                  std::string_view commitDevice,
                  sys::ProgressTask *task)
{
    fslib::File sourceFile(source, FsOpenMode_Read);
    fslib::File destinationFile(destination, FsOpenMode_Create | FsOpenMode_Write, sourceFile.getSize());
    if (!sourceFile || !destinationFile)
    {
        logger::log("Error opening one of the files: %s", fslib::getErrorString());
        return;
    }

    // Set status if task pointer was passed.
    if (task)
    {
        task->setStatus(strings::getByName(strings::names::COPYING_FILES, 0), source.cString());
    }

    // Shared struct both threads use
    std::shared_ptr<FileTransferStruct> sharedData(new FileTransferStruct);
    sharedData->m_readBuffer = std::make_unique<unsigned char[]>(FILE_BUFFER_SIZE);

    // To do: Static thread or pool to avoid reallocating thread.
    std::thread readThread(readThreadFunction, std::ref(sourceFile), sharedData);

    // This thread has a local buffer so the read thread can continue while this one writes.
    std::unique_ptr<unsigned char[]> localBuffer(new unsigned char[FILE_BUFFER_SIZE]);

    // Get file size for loop and set goal.
    int64_t fileSize = sourceFile.getSize();
    if (task)
    {
        task->reset(static_cast<double>(fileSize));
    }

    for (int64_t writeCount = 0, readCount = 0, journalCount = 0; writeCount < fileSize;)
    {
        {
            // Wait for lock/signal.
            std::unique_lock<std::mutex> m_bufferLock(sharedData->m_bufferLock);
            sharedData->m_bufferCondition.wait(m_bufferLock, [&sharedData]() { return sharedData->m_bufferIsFull; });

            // Record read count.
            readCount = sharedData->m_readSize;

            // Copy shared to local.
            std::memcpy(localBuffer.get(), sharedData->m_readBuffer.get(), readCount);

            // Signal buffer was copied and release mutex.
            sharedData->m_bufferIsFull = false;
            sharedData->m_bufferCondition.notify_one();
        }

        // Journaling size check. Breathing room is given.
        if (journalSize != 0 && (journalCount + readCount) >= static_cast<int64_t>(journalSize) - 0x100000)
        {
            // Reset journal count.
            journalCount = 0;
            // Close destination file, commit.
            destinationFile.close();
            fslib::commitDataToFileSystem(commitDevice);
            // Reopen and seek to previous position since we created it with a size earlier.
            destinationFile.open(destination, FsOpenMode_Write);
            destinationFile.seek(writeCount, destinationFile.beginning);
        }
        // Write to destination
        destinationFile.write(localBuffer.get(), readCount);
        // Update write and journal count.
        writeCount += readCount;
        journalCount += readCount;
        // Update task if passed.
        if (task)
        {
            task->updateCurrent(static_cast<double>(writeCount));
        }
    }
    // Wait for read thread and free it.
    readThread.join();
}

void fs::copyDirectory(const fslib::Path &source,
                       const fslib::Path &destination,
                       uint64_t journalSize,
                       std::string_view commitDevice,
                       sys::ProgressTask *task)
{
    fslib::Directory sourceDir(source);
    if (!sourceDir)
    {
        logger::log("Error opening directory for reading: %s", fslib::getErrorString());
        return;
    }

    for (int64_t i = 0; i < sourceDir.getCount(); i++)
    {
        if (sourceDir.isDirectory(i))
        {
            fslib::Path newSource = source / sourceDir[i];
            fslib::Path newDestination = destination / sourceDir[i];
            // Try to create new destination folder and continue loop on failure.
            if (!fslib::directoryExists(newDestination) && !fslib::createDirectory(newDestination))
            {
                logger::log("Error creating new destination directory: %s", fslib::getErrorString());
                continue;
            }

            fs::copyDirectory(newSource, newDestination, journalSize, commitDevice, task);
        }
        else
        {
            fslib::Path fullSource = source / sourceDir[i];
            fslib::Path fullDestination = destination / sourceDir[i];
            fs::copyFile(fullSource, fullDestination, journalSize, commitDevice, task);
        }
    }
}

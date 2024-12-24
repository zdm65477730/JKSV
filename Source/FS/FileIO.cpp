#include "FS/FileIO.hpp"
#include "Logger.hpp"
#include "Strings.hpp"
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
        std::mutex BufferLock;
        // Conditional to wait on signals.
        std::condition_variable BufferCondition;
        // Bool to control signals.
        bool BufferIsFull = false;
        // Number of bytes read.
        size_t ReadSize = 0;
        // Shared (read) buffer.
        std::unique_ptr<unsigned char[]> ReadBuffer;
} FileTransferStruct;

// This function Reads into the buffer. The other thread writes.
static void ReadThreadFunction(FsLib::File &SourceFile, std::shared_ptr<FileTransferStruct> SharedData)
{
    int64_t FileSize = SourceFile.GetSize();
    for (int64_t ReadCount = 0; ReadCount < FileSize;)
    {
        // Read data to shared buffer.
        SharedData->ReadSize = SourceFile.Read(SharedData->ReadBuffer.get(), FILE_BUFFER_SIZE);
        // Update local read count
        ReadCount += SharedData->ReadSize;
        // Signal to other thread buffer is full.
        SharedData->BufferIsFull = true;
        SharedData->BufferCondition.notify_one();
        // Wait for other thread to signal buffer is empty. Lock is released immediately, but it works and that's what matters.
        std::unique_lock<std::mutex> BufferLock(SharedData->BufferLock);
        SharedData->BufferCondition.wait(BufferLock, [&SharedData]() { return SharedData->BufferIsFull == false; });
    }
}

void FS::CopyFile(const FsLib::Path &Source,
                  const FsLib::Path &Destination,
                  uint64_t JournalSize,
                  std::string_view CommitDevice,
                  System::ProgressTask *Task)
{
    FsLib::File SourceFile(Source, FsOpenMode_Read);
    FsLib::File DestinationFile(Destination, FsOpenMode_Create | FsOpenMode_Write, SourceFile.GetSize());
    if (!SourceFile.IsOpen() || !DestinationFile.IsOpen())
    {
        Logger::Log("Error opening one of the files: %s", FsLib::GetErrorString());
        return;
    }

    // Set status if task pointer was passed.
    if (Task)
    {
        Task->SetStatus(Strings::GetByName(Strings::Names::CopyingFiles, 0), Source.CString());
    }

    // Shared struct both threads use
    std::shared_ptr<FileTransferStruct> SharedData(new FileTransferStruct);
    SharedData->ReadBuffer = std::make_unique<unsigned char[]>(FILE_BUFFER_SIZE);

    // To do: Static thread or pool to avoid reallocating thread.
    std::thread ReadThread(ReadThreadFunction, std::ref(SourceFile), SharedData);

    // This thread has a local buffer so the read thread can continue while this one writes.
    std::unique_ptr<unsigned char[]> LocalBuffer(new unsigned char[FILE_BUFFER_SIZE]);

    // Get file size for loop and set goal.
    int64_t FileSize = SourceFile.GetSize();
    if (Task)
    {
        Task->Reset(static_cast<double>(FileSize));
    }

    for (int64_t WriteCount = 0, ReadCount = 0, JournalCount = 0; WriteCount < FileSize;)
    {
        {
            // Wait for lock/signal.
            std::unique_lock<std::mutex> BufferLock(SharedData->BufferLock);
            SharedData->BufferCondition.wait(BufferLock, [&SharedData]() { return SharedData->BufferIsFull; });

            // Record read count.
            ReadCount = SharedData->ReadSize;

            // Copy shared to local.
            std::memcpy(LocalBuffer.get(), SharedData->ReadBuffer.get(), ReadCount);

            // Signal buffer was copied and release mutex.
            SharedData->BufferIsFull = false;
            SharedData->BufferCondition.notify_one();
        }

        // Journaling size check. Breathing room is given.
        if (JournalSize != 0 && (JournalCount + ReadCount) >= static_cast<int64_t>(JournalSize) - 0x100000)
        {
            // Reset journal count.
            JournalCount = 0;
            // Close Destination file, commit.
            DestinationFile.Close();
            FsLib::CommitDataToFileSystem(CommitDevice);
            // Reopen and seek to previous position since we created it with a size earlier.
            DestinationFile.Open(Destination, FsOpenMode_Write);
            DestinationFile.Seek(WriteCount, DestinationFile.Beginning);
        }
        // Write to destination
        DestinationFile.Write(LocalBuffer.get(), ReadCount);
        // Update write and journal count.
        WriteCount += ReadCount;
        JournalCount += ReadCount;
        // Update task if passed.
        if (Task)
        {
            Task->UpdateCurrent(static_cast<double>(WriteCount));
        }
    }
    // Wait for read thread and free it.
    ReadThread.join();
}

void FS::CopyDirectory(const FsLib::Path &Source,
                       const FsLib::Path &Destination,
                       uint64_t JournalSize,
                       std::string_view CommitDevice,
                       System::ProgressTask *Task)
{
    FsLib::Directory SourceDir(Source);
    if (!SourceDir.IsOpen())
    {
        Logger::Log("Error opening directory for reading: %s", FsLib::GetErrorString());
        return;
    }

    for (int64_t i = 0; i < SourceDir.GetEntryCount(); i++)
    {
        if (SourceDir.EntryAtIsDirectory(i))
        {
            FsLib::Path NewSource = Source / SourceDir[i];
            FsLib::Path NewDestination = Destination / SourceDir[i];
            // Try to create new destination folder and continue loop on failure.
            if (!FsLib::DirectoryExists(NewDestination) && !FsLib::CreateDirectory(NewDestination))
            {
                Logger::Log("Error creating new destination directory: %s", FsLib::GetErrorString());
                continue;
            }

            FS::CopyDirectory(NewSource, NewDestination, JournalSize, CommitDevice, Task);
        }
        else
        {
            FsLib::Path FullSource = Source / SourceDir[i];
            FsLib::Path FullDestination = Destination / SourceDir[i];
            FS::CopyFile(FullSource, FullDestination, JournalSize, CommitDevice, Task);
        }
    }
}

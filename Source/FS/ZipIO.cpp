#include "FS/ZipIO.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include "Strings.hpp"
#include <condition_variable>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>
#include <thread>

namespace
{
    // Size used for Zipping files.
    constexpr size_t ZIP_BUFFER_SIZE = 0x80000;
    // Size used for unzipping.
    constexpr size_t UNZIP_BUFFER_SIZE = 0x600000;
} // namespace

// Shared struct for Zip/File IO
typedef struct
{
        // Mutex and condition for buffer.
        std::mutex BufferLock;
        std::condition_variable BufferCondition;
        bool BufferIsFull = false;
        // Number of bytes read from file.
        ssize_t ReadCount = 0;
        // Shared/reading buffer.
        std::unique_ptr<unsigned char[]> SharedBuffer;
} ZipIOStruct;

// Function for reading files for Zipping.
void ZipReadThreadFunction(FsLib::File &Source, std::shared_ptr<ZipIOStruct> SharedData)
{
    int64_t FileSize = Source.GetSize();
    for (int64_t ReadCount = 0; ReadCount < FileSize;)
    {
        // Read into shared buffer.
        SharedData->ReadCount = Source.Read(SharedData->SharedBuffer.get(), ZIP_BUFFER_SIZE);
        // Update read count
        ReadCount += SharedData->ReadCount;
        // Signal other thread buffer is ready to go.
        SharedData->BufferIsFull = true;
        SharedData->BufferCondition.notify_one();
        // Wait for other thread to release lock on buffer so this thread can read again.
        std::unique_lock<std::mutex> BufferLock(SharedData->BufferLock);
        SharedData->BufferCondition.wait(BufferLock, [&SharedData]() { return SharedData->BufferIsFull == false; });
    }
}

// Function for reading data from Zip to buffer.
void UnzipReadThreadFunction(unzFile Source, int64_t FileSize, std::shared_ptr<ZipIOStruct> SharedData)
{
    for (int64_t ReadCount = 0; ReadCount < FileSize;)
    {
        ReadCount = unzReadCurrentFile(Source, SharedData->SharedBuffer.get(), UNZIP_BUFFER_SIZE);
    }
}

void FS::CopyDirectoryToZip(const FsLib::Path &Source, zipFile Destination, System::ProgressTask *Task)
{
    FsLib::Directory SourceDir(Source);
    if (!SourceDir.IsOpen())
    {
        Logger::Log("Error opening source directory: %s", FsLib::GetErrorString());
        return;
    }

    for (int64_t i = 0; i < SourceDir.GetEntryCount(); i++)
    {
        if (SourceDir.EntryAtIsDirectory(i))
        {
            FsLib::Path NewSource = Source / SourceDir[i];
            FS::CopyDirectoryToZip(NewSource, Destination, Task);
        }
        else
        {
            // Open source file.
            FsLib::Path FullSource = Source / SourceDir[i];
            FsLib::File SourceFile(FullSource, FsOpenMode_Read);
            if (!SourceFile.IsOpen())
            {
                Logger::Log("Error zipping file: %s", FsLib::GetErrorString());
                continue;
            }

            // Date for file.
            std::time_t Timer;
            std::time(&Timer);
            std::tm *LocalTime = std::localtime(&Timer);
            zip_fileinfo FileInfo = {.tmz_date = {.tm_sec = LocalTime->tm_sec,
                                                  .tm_min = LocalTime->tm_min,
                                                  .tm_hour = LocalTime->tm_hour,
                                                  .tm_mday = LocalTime->tm_mday,
                                                  .tm_mon = LocalTime->tm_mon,
                                                  .tm_year = LocalTime->tm_year + 1900},
                                     .dosDate = 0,
                                     .internal_fa = 0,
                                     .external_fa = 0};

            // Create new file in zip
            const char *FileNameBegin = std::strchr(FullSource.GetPath(), '/') + 1;
            int ZipError = zipOpenNewFileInZip64(Destination,
                                                 FileNameBegin,
                                                 &FileInfo,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 Z_DEFLATED,
                                                 Config::GetByKey(Config::Keys::ZipCompressionLevel),
                                                 1);
            if (ZipError != ZIP_OK)
            {
                Logger::Log("Error creating file in zip: %i.", ZipError);
                continue;
            }

            // Shared data for thread.
            std::shared_ptr<ZipIOStruct> SharedData(new ZipIOStruct);
            SharedData->SharedBuffer = std::make_unique<unsigned char[]>(ZIP_BUFFER_SIZE);

            // Local buffer for writing.
            std::unique_ptr<unsigned char[]> LocalBuffer(new unsigned char[ZIP_BUFFER_SIZE]);

            // Update task if passed.
            if (Task)
            {
                Task->SetStatus(Strings::GetByName(Strings::Names::CopyingFiles, 1), FullSource.CString());
                Task->Reset(static_cast<double>(SourceFile.GetSize()));
            }

            std::thread ReadThread(ZipReadThreadFunction, std::ref(SourceFile), SharedData);

            int64_t FileSize = SourceFile.GetSize();
            for (int64_t WriteCount = 0, ReadCount = 0; WriteCount < FileSize;)
            {
                {
                    // Wait for buffer signal
                    std::unique_lock<std::mutex> BufferLock(SharedData->BufferLock);
                    SharedData->BufferCondition.wait(BufferLock, [&SharedData]() { return SharedData->BufferIsFull; });

                    // Save read count, copy shared to local.
                    ReadCount = SharedData->ReadCount;
                    std::memcpy(LocalBuffer.get(), SharedData->SharedBuffer.get(), ReadCount);

                    // Signal copy was good and release lock.
                    SharedData->BufferIsFull = false;
                    SharedData->BufferCondition.notify_one();
                }
                // Write
                ZipError = zipWriteInFileInZip(Destination, LocalBuffer.get(), ReadCount);
                if (ZipError != ZIP_OK)
                {
                    Logger::Log("Error writing data to zip: %i.", ZipError);
                }
                // Update count and status
                WriteCount += ReadCount;
                if (Task)
                {
                    Task->UpdateCurrent(static_cast<double>(WriteCount));
                }
            }
            // Wait for thread
            ReadThread.join();
            // Close file in zip
            zipCloseFileInZip(Destination);
        }
    }
}

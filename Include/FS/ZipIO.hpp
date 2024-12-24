#pragma once
// Major to do: Stop using minizip and finish the ZipFile class.
#include "FsLib.hpp"
#include "System/ProgressTask.hpp"
#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <string_view>

namespace FS
{
    // Copies directory recursively into Destination.
    void CopyDirectoryToZip(const FsLib::Path &Source, zipFile Destination, System::ProgressTask *Task = nullptr);
    // Recursively copies Source to destination. This is only used for unzipping saves.
    void CopyZipToDirectory(unzFile Source,
                            const FsLib::Path &Destination,
                            uint64_t JournalSize,
                            std::string_view CommitDevice,
                            System::ProgressTask *Task = nullptr);
} // namespace FS

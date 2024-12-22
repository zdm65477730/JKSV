#pragma once
// Major to do: Stop using minizip and finish the ZipFile class.
#include "FsLib.hpp"
#include "System/ProgressTask.hpp"
#include <minizip/unzip.h>
#include <minizip/zip.h>

namespace FS
{
    // Copies directory recursively into Destination.
    void CopyDirectoryToZip(const FsLib::Path &Source, zipFile Destination, System::ProgressTask *Task = nullptr);
    // Recursively copies Source to destination.
    void CopyZipToDirectory(unzFile Source, const FsLib::Path &Destination, uint64_t JournalSize, System::ProgressTask *Task = nullptr);
} // namespace FS

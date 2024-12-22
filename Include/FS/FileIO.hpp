#pragma once
#include "FsLib.hpp"
#include "System/ProgressTask.hpp"
#include <string_view>

namespace FS
{
    // Copies the file. Task is optional, but if passed allows updating the progress of the current operation.
    void CopyFile(const FsLib::Path &Source,
                  const FsLib::Path &Destination,
                  uint64_t JournalSize = 0,
                  std::string_view CommitDevice = {},
                  System::ProgressTask *Task = nullptr);
    // Recursively copies Source to Destination.
    void CopyDirectory(const FsLib::Path &Source,
                       const FsLib::Path &Destination,
                       uint64_t JournalSize = 0,
                       std::string_view CommitDevice = {},
                       System::ProgressTask *Task = nullptr);
} // namespace FS

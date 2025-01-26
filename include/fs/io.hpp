#pragma once
#include "fslib.hpp"
#include "system/ProgressTask.hpp"
#include <string_view>

namespace fs
{
    /// @brief Copies source to destination.
    /// @param source Path to source file.
    /// @param destination Path to destination.
    /// @param journalSize Optional. The size of the journal if data needs to be commited.
    /// @param commitDevice Optional. The device to commit to if it's needed.
    /// @param Task Optional. Progress tracking task to display progress of operation if needed.
    void copyFile(const fslib::Path &source,
                  const fslib::Path &destination,
                  uint64_t journalSize = 0,
                  std::string_view commitDevice = {},
                  sys::ProgressTask *Task = nullptr);

    /// @brief Recursively copies source to destination.
    /// @param source Source path.
    /// @param destination Destination path.
    /// @param journalSize Optional. Journal size to be passed to copyFile if data needs to be commited to device.
    /// @param commitDevice Optional. Device to commit data to if needed.
    /// @param Task Option. Progress tracking task to be passed to copyFile to show progress of operation.
    void copyDirectory(const fslib::Path &source,
                       const fslib::Path &destination,
                       uint64_t journalSize = 0,
                       std::string_view commitDevice = {},
                       sys::ProgressTask *Task = nullptr);
} // namespace fs

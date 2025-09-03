#pragma once
// Major to do: Stop using minizip and finish the ZipFile class.
#include "fs/MiniUnzip.hpp"
#include "fs/MiniZip.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"

#include <string_view>

namespace fs
{
    /// @brief Copies source to destination.
    /// @note Task is optional.
    void copy_directory_to_zip(const fslib::Path &source, fs::MiniZip &dest, sys::ProgressTask *Task = nullptr);

    /// @brief Unzips source to destination.
    /// @note Task is optional.
    void copy_zip_to_directory(fs::MiniUnzip &source,
                               const fslib::Path &dest,
                               int64_t journalSize,
                               sys::ProgressTask *Task = nullptr);

    /// @brief Returns whether or not zip has files inside besides the save meta.
    bool zip_has_contents(const fslib::Path &zipPath);
} // namespace fs

#pragma once
// Major to do: Stop using minizip and finish the ZipFile class.
#include "fslib.hpp"
#include "system/ProgressTask.hpp"
#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <string_view>

namespace fs
{
    /// @brief Copies source to destination.
    /// @param source Source file to copy from.
    /// @param destination zipFile to write to.
    /// @param Task Optional. Task to pass to show progress.
    void copy_directory_to_zip(const fslib::Path &source, zipFile destination, sys::ProgressTask *Task = nullptr);

    /// @brief Unzips source to destination.
    /// @param source Source zip file to read from.
    /// @param destination Destination path to write to.
    /// @param journalSize Size of journal for committing data. This is used exclusively for save data.
    /// @param commitDevice Device to commit data to.
    /// @param task Optional. Task to update to show progress.
    void copy_zip_to_directory(unzFile source,
                               const fslib::Path &destination,
                               uint64_t journalSize,
                               std::string_view commitDevice,
                               sys::ProgressTask *Task = nullptr);

    /// @brief Returns whether or not zip has files inside.
    /// @param zipPath Path to zip to check.
    /// @return True if at least one file is found. False if none.
    bool zip_has_contents(const fslib::Path &zipPath);
} // namespace fs

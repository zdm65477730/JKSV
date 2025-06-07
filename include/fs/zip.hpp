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

    /// @brief Gets a filled zip_fileinfo struct.
    /// @param info Reference to the info struct to fill.
    void create_zip_fileinfo(zip_fileinfo &info);

    /// @brief Returns whether or not zip has files inside.
    /// @param zipPath Path to zip to check.
    /// @return True if at least one file is found. False if none.
    bool zip_has_contents(const fslib::Path &zipPath);

    /// @brief Attempts to locate a file in a zip file. If it's found, the current file is set to it.
    /// @param zip Zip file to search.
    /// @param name
    /// @return True if the file is found. False if it's not.
    bool locate_file_in_zip(unzFile zip, std::string_view name);

    /// @brief Gets the total uncompressed size of the files inside the zip file.
    /// @param zip Zip file to get the total size of.
    /// @return Total size of the zip file passed.
    /// @note unzFile passed is set to the first file afterwards.
    uint64_t get_zip_total_size(unzFile zip);
} // namespace fs

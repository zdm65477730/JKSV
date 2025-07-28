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
    void copy_file(const fslib::Path &source, const fslib::Path &destination, sys::ProgressTask *Task = nullptr);

    /// @brief Same as a above. Committing data to the device passed while copying.
    /// @param device Device to commit data to.
    /// @param journalSize Size of the journal area of the save data.
    void copy_file_commit(const fslib::Path &source,
                          const fslib::Path &destination,
                          std::string_view device,
                          int64_t journalSize,
                          sys::ProgressTask *task = nullptr);

    /// @brief Recursively copies source to destination.
    /// @param source Source path.
    /// @param destination Destination path.
    /// @param journalSize Optional. Journal size to be passed to copyFile if data needs to be commited to device.
    /// @param commitDevice Optional. Device to commit data to if needed.
    /// @param Task Option. Progress tracking task to be passed to copyFile to show progress of operation.
    void copy_directory(const fslib::Path &source, const fslib::Path &destination, sys::ProgressTask *Task = nullptr);

    /// @brief Same as above but committing data passed while copying.
    /// @param device Device to commit data to.
    /// @param journalSize Size of the journaling area of the save.
    void copy_directory_commit(const fslib::Path &source,
                               const fslib::Path &destination,
                               std::string_view device,
                               int64_t journalSize,
                               sys::ProgressTask *task = nullptr);

} // namespace fs

#pragma once
#include "fslib.hpp"

namespace fs
{
    /// @brief Recursively runs through the path passed and retrieves information.
    /// @param directoryPath Path of directory.
    /// @param subDirCount Int64_t to track number of subdirectories with.
    /// @param fileCount Int64_t to track the number of files.
    /// @param totalSize Int64_t to track the size of everything.
    /// @return True on success. False or crash on what I would assume is a stack overflow.
    bool get_directory_information(const fslib::Path &directoryPath,
                                   int64_t &subDirCount,
                                   int64_t &fileCount,
                                   int64_t &totalSize);

    /// @brief Checks if directory is empty. Didn't feel like this needs its own source file.
    /// @param directoryPath Path to directory to check.
    /// @return True if directory has files inside.
    bool directory_has_contents(const fslib::Path &directoryPath);

    /// @brief Recursively moves (renames) everything in oldPath to newPath.
    bool move_directory_recursively(const fslib::Path &oldPath, const fslib::Path &newPath);
} // namespace fs

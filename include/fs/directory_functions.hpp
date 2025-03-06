#pragma once
#include "fslib.hpp"

namespace fs
{
    /// @brief Retrieves the total size of the contents of the directory at targetPath.
    /// @param targetPath Directory to calculate.
    uint64_t get_directory_total_size(const fslib::Path &targetPath);

    /// @brief Checks if directory is empty. Didn't feel like this needs its own source file.
    /// @param directoryPath Path to directory to check.
    /// @return True if directory has files inside.
    bool directory_has_contents(const fslib::Path &directoryPath);
} // namespace fs

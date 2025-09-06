#include "fs/directory_functions.hpp"

#include "error.hpp"
#include "fs/SaveMetaData.hpp"
#include "logging/logger.hpp"

bool fs::get_directory_information(const fslib::Path &directoryPath,
                                   int64_t &subDirCount,
                                   int64_t &fileCount,
                                   int64_t &totalSize)
{
    fslib::Directory dir{directoryPath};
    if (error::fslib(dir.is_open())) { return false; }

    for (const fslib::DirectoryEntry &entry : dir)
    {
        if (entry.is_directory())
        {
            const fslib::Path newPath{directoryPath / entry};
            const bool getInfo = fs::get_directory_information(newPath, subDirCount, fileCount, totalSize);
            if (!getInfo) { return false; }
            ++subDirCount;
        }
        else
        {
            totalSize += entry.get_size();
            ++fileCount;
        }
    }

    return true;
}

bool fs::directory_has_contents(const fslib::Path &directoryPath)
{
    fslib::Directory testDir{directoryPath};
    if (!testDir.is_open()) { return false; }

    for (const fslib::DirectoryEntry &entry : testDir)
    {
        if (entry.get_filename() != fs::NAME_SAVE_META) { return true; }
    }

    return false;
}

#include "fs/directory_functions.hpp"

#include "fs/SaveMetaData.hpp"

uint64_t fs::get_directory_total_size(const fslib::Path &targetPath)
{
    fslib::Directory targetDir{targetPath};
    if (!targetDir.is_open()) { return 0; }

    uint64_t directorySize = 0;
    for (const fslib::DirectoryEntry &entry : targetDir)
    {
        if (entry.is_directory())
        {
            const fslib::Path newTarget{targetPath / entry};
            directorySize += fs::get_directory_total_size(newTarget);
        }
        else { directorySize += entry.get_size(); }
    }

    return directorySize;
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

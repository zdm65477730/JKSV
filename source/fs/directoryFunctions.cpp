#include "fs/directoryFunctions.hpp"

uint64_t fs::get_directory_total_size(const fslib::Path &targetPath)
{
    fslib::Directory targetDir(targetPath);
    if (!targetDir)
    {
        return 0;
    }

    uint64_t directorySize = 0;
    for (int64_t i = 0; i < targetDir.getCount(); i++)
    {
        if (targetDir.isDirectory(i))
        {
            fslib::Path newTarget = targetPath / targetDir[i];
            directorySize += get_directory_total_size(newTarget);
        }
        else
        {
            directorySize += targetDir.getEntrySize(i);
        }
    }
    return directorySize;
}

bool fs::directory_has_contents(const fslib::Path &directoryPath)
{
    fslib::Directory testDir(directoryPath);
    if (!testDir)
    {
        return false;
    }
    return testDir.getCount() != 0;
}

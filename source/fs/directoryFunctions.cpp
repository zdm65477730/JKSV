#include "fs/directoryFunctions.hpp"

uint64_t fs::getDirectoryTotalSize(const fslib::Path &targetPath)
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
            directorySize += getDirectoryTotalSize(newTarget);
        }
        else
        {
            directorySize += targetDir.getEntrySize(i);
        }
    }
    return directorySize;
}

bool fs::directoryHasContents(const fslib::Path &directoryPath)
{
    fslib::Directory testDir(directoryPath);
    if (!testDir)
    {
        return false;
    }
    return testDir.getCount() != 0;
}

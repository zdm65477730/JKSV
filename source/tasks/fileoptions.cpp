#include "tasks/fileoptions.hpp"

#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"

void tasks::fileoptions::copy_source_to_destination(sys::ProgressTask *task, FileOptionState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    fslib::Path &source  = taskData->sourcePath;
    fslib::Path &dest    = taskData->destPath;
    int64_t journalSpace = taskData->journalSize;

    const bool isDir       = fslib::directory_exists(source);
    const bool needsCommit = journalSpace > 0;

    if (isDir && needsCommit) { fs::copy_directory_commit(source, dest, journalSpace, task); }
    else if (!isDir && needsCommit) { fs::copy_file_commit(source, dest, journalSpace, task); }
    else if (isDir && !needsCommit) { fs::copy_directory(source, dest, task); }
    else if (!isDir && !needsCommit) { fs::copy_file(source, dest, task); }

    task->complete();
}

void tasks::fileoptions::delete_target(sys::Task *task, FileOptionState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    fslib::Path target   = taskData->sourcePath;
    int64_t journalSpace = taskData->journalSize;

    const bool isDir = fslib::directory_exists(target);
    bool needsCommit = journalSpace > 0;

    if (isDir) { fslib::delete_directory_recursively(target); }
    else { fslib::delete_file(target); }

    if (needsCommit) { fslib::commit_data_to_file_system(target.get_device_name()); }

    task->complete();
}
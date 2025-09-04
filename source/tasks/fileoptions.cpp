#include "tasks/fileoptions.hpp"

#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "logging/logger.hpp"

void tasks::fileoptions::copy_source_to_destination(sys::ProgressTask *task, FileOptionState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    const fslib::Path &source      = taskData->sourcePath;
    const fslib::Path &dest        = taskData->destPath;
    const int64_t journalSpace     = taskData->journalSize;
    FileOptionState *spawningState = taskData->spawningState;

    const bool sourceIsDir = fslib::directory_exists(source);
    bool destError         = false;
    if (sourceIsDir) { destError = error::fslib(fslib::create_directories_recursively(dest)); }
    else
    {
        const size_t subDest = dest.find_last_of('/');
        if (subDest != dest.NOT_FOUND && subDest > 1)
        {
            fslib::Path subDestPath{dest.sub_path(subDest)};
            destError = error::fslib(fslib::create_directories_recursively(subDestPath));
        }
    }

    if (destError) { TASK_FINISH_RETURN(task); }

    const bool needsCommit = journalSpace > 0;

    if (sourceIsDir && needsCommit) { fs::copy_directory_commit(source, dest, journalSpace, task); }
    else if (!sourceIsDir && needsCommit) { fs::copy_file_commit(source, dest, journalSpace, task); }
    else if (sourceIsDir && !needsCommit) { fs::copy_directory(source, dest, task); }
    else if (!sourceIsDir && !needsCommit) { fs::copy_file(source, dest, task); }

    spawningState->update_destination();
    task->complete();
}

void tasks::fileoptions::delete_target(sys::Task *task, FileOptionState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    fslib::Path target             = taskData->sourcePath;
    int64_t journalSpace           = taskData->journalSize;
    FileOptionState *spawningState = taskData->spawningState;

    const bool isDir = fslib::directory_exists(target);
    bool needsCommit = journalSpace > 0;

    if (isDir) { fslib::delete_directory_recursively(target); }
    else { fslib::delete_file(target); }

    if (needsCommit) { fslib::commit_data_to_file_system(target.get_device_name()); }

    spawningState->update_source();
    task->complete();
}
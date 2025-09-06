#include "tasks/fileoptions.hpp"

#include "appstates/MessageState.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"

void tasks::fileoptions::copy_source_to_destination(sys::ProgressTask *task, FileOptionState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    const int popTicks             = ui::PopMessageManager::DEFAULT_TICKS;
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

    if (destError)
    {
        const char *errorFormat = strings::get_by_name(strings::names::FILEOPTION_POPS, 0);
        const std::string pop   = stringutil::get_formatted_string(errorFormat, source.get_filename());
        ui::PopMessageManager::push_message(popTicks, pop);
        TASK_FINISH_RETURN(task);
    }

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

    // Gonna borrow this. No point in duplicating it.
    const int popTicks         = ui::PopMessageManager::DEFAULT_TICKS;
    const char *deletingFormat = strings::get_by_name(strings::names::IO_STATUSES, 3);
    const char *errorFormat    = strings::get_by_name(strings::names::FILEOPTION_POPS, 1);

    const fslib::Path &target      = taskData->sourcePath;
    const int64_t journalSpace     = taskData->journalSize;
    FileOptionState *spawningState = taskData->spawningState;

    {
        const char *filename     = target.get_filename();
        const std::string status = stringutil::get_formatted_string(deletingFormat, filename);
        task->set_status(status);
    }

    const bool isDir = fslib::directory_exists(target);
    bool needsCommit = journalSpace > 0;

    bool deleteError{};
    if (isDir) { deleteError = error::fslib(fslib::delete_directory_recursively(target)); }
    else { deleteError = error::fslib(fslib::delete_file(target)); }

    if (deleteError)
    {
        const char *filename  = target.get_filename();
        const std::string pop = stringutil::get_formatted_string(errorFormat, filename);
        ui::PopMessageManager::push_message(popTicks, pop);
    }

    const bool commitError = needsCommit && error::fslib(fslib::commit_data_to_file_system(target.get_device_name()));
    if (commitError)
    {
        const char *filename  = target.get_filename();
        const std::string pop = stringutil::get_formatted_string(errorFormat, filename);
        ui::PopMessageManager::push_message(popTicks, pop);
    }

    spawningState->update_source();
    task->complete();
}

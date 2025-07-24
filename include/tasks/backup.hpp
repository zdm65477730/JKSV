#pragma once
#include "appstates/BackupMenuState.hpp"
#include "system/ProgressTask.hpp"
#include "system/Task.hpp"

namespace tasks
{
    namespace backup
    {
        /// @brief Task/thread function executed when a new backup is created.
        void create_new_backup(sys::ProgressTask *task,
                               data::User *user,
                               data::TitleInfo *titleInfo,
                               fslib::Path target,
                               BackupMenuState *spawningState,
                               bool killTask = true);

        /// @brief Overwrites a pre-existing backup.
        void overwrite_backup(sys::ProgressTask *task, BackupMenuState::TaskData taskData);

        /// @brief Restores a backup
        void restore_backup(sys::ProgressTask *task, BackupMenuState::TaskData taskData);

        /// @brief Deletes a backup
        void delete_backup(sys::Task *task, BackupMenuState::TaskData taskData);

        /// @brief Uploads a backup
        void upload_backup(sys::ProgressTask *task, BackupMenuState::TaskData taskData);
    }
}

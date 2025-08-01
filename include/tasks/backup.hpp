#pragma once
#include "appstates/BackupMenuState.hpp"
#include "sys/sys.hpp"

#include <string>

namespace tasks
{
    namespace backup
    {
        /// @brief Task/thread function executed when a new backup is created.
        void create_new_backup_local(sys::ProgressTask *task,
                                     data::User *user,
                                     data::TitleInfo *titleInfo,
                                     fslib::Path target,
                                     BackupMenuState *spawningState,
                                     bool killTask = true);
        void create_new_backup_remote(sys::ProgressTask *task,
                                      data::User *user,
                                      data::TitleInfo *titleInfo,
                                      std::string remoteName,
                                      BackupMenuState *spawningState,
                                      bool killTask = true);

        /// @brief Overwrites a pre-existing backup.
        void overwrite_backup_local(sys::ProgressTask *task, BackupMenuState::TaskData taskData);
        void overwrite_backup_remote(sys::ProgressTask *task, BackupMenuState::TaskData taskData);

        /// @brief Restores a backup
        void restore_backup_local(sys::ProgressTask *task, BackupMenuState::TaskData taskData);
        void restore_backup_remote(sys::ProgressTask *task, BackupMenuState::TaskData taskData);

        /// @brief Deletes a backup
        void delete_backup_local(sys::Task *task, BackupMenuState::TaskData taskData);
        void delete_backup_remote(sys::Task *task, BackupMenuState::TaskData taskData);

        /// @brief Uploads a backup
        void upload_backup(sys::ProgressTask *task, BackupMenuState::TaskData taskData);

        /// @brief Patches a pre-existing backup on the remote storage.
        void patch_backup(sys::ProgressTask *task, BackupMenuState::TaskData taskData);
    }
}

#pragma once

#include "appstates/TitleOptionState.hpp"
#include "sys/sys.hpp"

namespace tasks::titleoptions
{
    /// @brief Adds a title to the blacklist. Needs to be task formatted to work with confirmations.
    void blacklist_title(sys::Task *task, TitleOptionState::TaskData taskData);

    /// @brief Wipes deletes all local backups for the current title.
    void delete_all_local_backups_for_title(sys::Task *task, TitleOptionState::TaskData taskData);

    /// @brief Deletes all backups found on the remote storage service.
    void delete_all_remote_backups_for_title(sys::Task *task, TitleOptionState::TaskData taskData);

    /// @brief Resets save data for the current title.
    void reset_save_data(sys::Task *task, TitleOptionState::TaskData taskData);

    /// @brief Deletes the save data from the system the same way Data Management does.
    void delete_save_data_from_system(sys::Task *task, TitleOptionState::TaskData taskData);

    /// @brief Extends the save container for the current save info.
    void extend_save_data(sys::Task *task, TitleOptionState::TaskData taskData);
}

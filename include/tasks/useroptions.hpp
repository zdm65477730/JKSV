#pragma once
#include "appstates/UserOptionState.hpp"
#include "sys/sys.hpp"

namespace tasks
{
    namespace useroptions
    {
        void backup_all_for_user_local(sys::ProgressTask *task, UserOptionState::TaskData taskData);
        void backup_all_for_user_remote(sys::ProgressTask *task, UserOptionState::TaskData taskData);
        void create_all_save_data_for_user(sys::Task *task, UserOptionState::TaskData taskData);
        void delete_all_save_data_for_user(sys::Task *task, UserOptionState::TaskData taskData);
    }
}

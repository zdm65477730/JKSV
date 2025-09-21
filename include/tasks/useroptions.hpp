#pragma once
#include "appstates/UserOptionState.hpp"
#include "sys/sys.hpp"

namespace tasks::useroptions
{
    void backup_all_for_user_local(sys::threadpool::JobData taskData);
    void backup_all_for_user_remote(sys::threadpool::JobData taskData);
    void create_all_save_data_for_user(sys::threadpool::JobData taskData);
    void delete_all_save_data_for_user(sys::threadpool::JobData taskData);
}

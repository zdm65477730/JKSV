#pragma once
#include "appstates/SaveCreateState.hpp"
#include "data/data.hpp"
#include "sys/sys.hpp"

namespace tasks::savecreate
{
    /// @brief Default save creating task.
    /// @param taskData Data to pass to thread.
    void create_save_data_for(sys::threadpool::JobData taskData);

    /// @brief Same as above, but creates the save on the SD card instead. Only used for cache type saves.
    /// @param taskData Data to pass to thread.kl;'
    void create_save_data_for_sd(sys::threadpool::JobData taskData);
}

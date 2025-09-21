#pragma once
#include "appstates/SaveCreateState.hpp"
#include "data/data.hpp"
#include "sys/sys.hpp"

namespace tasks::savecreate
{
    void create_save_data_for(sys::threadpool::JobData taskData);
}

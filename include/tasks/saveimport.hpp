#pragma once
#include "sys/Task.hpp"

namespace tasks::saveimport
{
    /// @brief Imports a random save backup according to the meta data.
    void import_save_backup(sys::threadpool::JobData taskData);
}

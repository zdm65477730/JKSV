#pragma once
#include "sys/threadpool.hpp"

namespace tasks::update
{
    /// @brief Checks for updates. If one is found, prompts to download it.
    void check_for_update(sys::threadpool::JobData jobData);

    /// @brief Downloads the new JKSV NRO.
    void download_update(sys::threadpool::JobData jobData);
}

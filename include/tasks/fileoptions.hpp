#pragma once
#include "appstates/FileOptionState.hpp"
#include "sys/sys.hpp"

namespace tasks::fileoptions
{
    /// @brief Copies the source to destination passed through taskData.
    void copy_source_to_destination(sys::ProgressTask *task, FileOptionState::TaskData taskData);

    /// @brief Deletes the source path passed through taskData
    void delete_target(sys::Task *task, FileOptionState::TaskData taskData);
}
#pragma once
#include "data/data.hpp"
#include <switch.h>

namespace fs
{
    /// @brief Creates save data for the target user for the title passed.
    /// @param targetUser User to create save data for.
    /// @param titleInfo Title to create save data for.
    /// @return True on success. False on failure.
    bool create_save_data_for(data::User *targetUser, data::TitleInfo *titleInfo);

    /// @brief Deletes the save data of the FsSaveDataInfo passed.
    /// @param saveInfo Save data to delete.
    /// @return True on success. False on failure.
    bool delete_save_data(const FsSaveDataInfo &saveInfo);
} // namespace fs

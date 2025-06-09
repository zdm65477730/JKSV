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
    bool delete_save_data(const FsSaveDataInfo *saveInfo);

    /// @brief Extends the save data of the FsSaveDataInfo struct passed.
    /// @param saveInfo Pointer to the FsSaveDataInfo struct of the save to extend.
    /// @param size Size (in MB) to extend the save data to.
    /// @param journalSize Size of the journaling space.
    /// @return True on success. False on failure.
    bool extend_save_data(const FsSaveDataInfo *saveInfo, int64_t size, int64_t journalSize);

    /// @brief Returns whether or not the saveInfo passed is system type.
    /// @param saveInfo FsSaveDataInfo to check.
    /// @return True if it is. False if it isn't.
    /// @note The config setting overrides this.
    bool is_system_save_data(const FsSaveDataInfo *saveInfo);
} // namespace fs

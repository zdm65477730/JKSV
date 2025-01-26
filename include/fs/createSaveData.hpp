#pragma once
#include "data/data.hpp"

namespace fs
{
    /// @brief Creates save data for the target user for the title passed.
    /// @param targetUser User to create save data for.
    /// @param titleInfo Title to create save data for.
    /// @return True on success. False on failure.
    bool createSaveDataFor(data::User *targetUser, data::TitleInfo *titleInfo);
} // namespace fs

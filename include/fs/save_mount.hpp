#pragma once
#include <string_view>
#include <switch.h>

namespace fs
{
    /// @brief Default mount point used for JKSV for saves.
    inline constexpr std::string_view DEFAULT_SAVE_MOUNT = "save";

    /// @brief Same as above, but as a root directory.
    inline constexpr std::string_view DEFAULT_SAVE_ROOT = "save:/";
} // namespace fs

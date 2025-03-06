#pragma once
#include <string_view>
#include <switch.h>

namespace fs
{
    /// @brief Default mount point used for JKSV for saves.
    static constexpr std::string_view DEFAULT_SAVE_MOUNT = "save";

    /// @brief Same as above, but as a root directory.
    static constexpr std::string_view DEFAULT_SAVE_PATH = "save:/";
} // namespace fs

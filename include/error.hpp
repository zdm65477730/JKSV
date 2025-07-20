#pragma once
#include <source_location>
#include <switch.h>

namespace error
{
    /// @brief Logs and returns if a call from libnx fails.
    bool libnx(Result code, const std::source_location &location = std::source_location::current());

    /// @brief Logs and returns if an fslib function fails.
    bool fslib(bool result, const std::source_location &location = std::source_location::current());
}

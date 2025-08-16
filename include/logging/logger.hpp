#pragma once
#include <string_view>

namespace logger
{
    /// @brief Creates the log file if it doesn't exist already.
    void initialize();

    /// @brief Logs a formatted string.
    /// @param format Format of string.
    /// @param arguments Va arguments.
    void log(const char *format, ...);
} // namespace logger

#pragma once
#include <string_view>

namespace logger
{
    /// @brief Creates and empties the log file
    void initialize();

    /// @brief Logs a formatted string.
    /// @param format Format of string.
    /// @param arguments Va arguments.
    void log(const char *format, ...);

    void log_straight(std::string_view string);
} // namespace logger

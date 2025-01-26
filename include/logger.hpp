#pragma once

namespace logger
{
    /// @brief Creates and empties the log file
    void initialize(void);

    /// @brief Logs a formatted string.
    /// @param format Format of string.
    /// @param arguments Va arguments.
    void log(const char *format, ...);
} // namespace logger

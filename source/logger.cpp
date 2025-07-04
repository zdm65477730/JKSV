#include "logger.hpp"
#include "config.hpp"
#include "fslib.hpp"
#include <cstdarg>

namespace
{
    /// @brief This is the path to the log file.
    fslib::Path s_logFilePath;

    /// @brief This is the buffer size for log strings.
    constexpr size_t VA_BUFFER_SIZE = 0x1000;
} // namespace

void logger::initialize()
{
    // Create log path and empty the log for this run.
    s_logFilePath = "sdmc:/config/JKSV/JKSV.log";

    // Just opening it like this to nuke and restart.
    fslib::File LogFile(s_logFilePath, FsOpenMode_Create | FsOpenMode_Write);
}

void logger::log(const char *format, ...)
{
    char vaBuffer[VA_BUFFER_SIZE] = {0};

    std::va_list vaList;
    va_start(vaList, format);
    vsnprintf(vaBuffer, VA_BUFFER_SIZE, format, vaList);
    va_end(vaList);

    fslib::File logFile(s_logFilePath, FsOpenMode_Append);
    logFile << vaBuffer << "\n";

    // Always flush to guarantee output.
    logFile.flush();
}

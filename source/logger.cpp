#include "logger.hpp"
#include "config.hpp"
#include "fslib.hpp"
#include <cstdarg>

namespace
{
    // Path to log file.
    fslib::Path s_logFilePath;
    // Size of va buffer for log.
    constexpr size_t VA_BUFFER_SIZE = 0x1000;
} // namespace

void logger::initialize(void)
{
    // Create log path and empty the log for this run.
    s_logFilePath = "sdmc:/switch/JKSV.log";
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
    logFile.flush();
}

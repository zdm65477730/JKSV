#include "logging/logger.hpp"

#include "config/config.hpp"
#include "fslib.hpp"

#include <array>
#include <cstdarg>
#include <fstream>
#include <mutex>
#include <switch.h>

namespace
{
    constexpr const char *PATH_JKSV_LOG = "sdmc:/config/JKSV/JKSV.log";

    /// @brief This is the buffer size for log strings.
    constexpr size_t VA_BUFFER_SIZE = 0x1000;

    constexpr int64_t SIZE_LOG_LIMIT = 0x10000;
} // namespace

void logger::initialize()
{
    // Can't really log errors for the log before it exists?
    const fslib::Path logPath{PATH_JKSV_LOG};
    const bool exists = fslib::file_exists(logPath);
    if (!exists) { fslib::create_file(logPath); }
}

void logger::log(const char *format, ...)
{
    static std::mutex logLock{};

    std::array<char, VA_BUFFER_SIZE> vaBuffer = {};
    std::va_list vaList{};
    va_start(vaList, format);
    vsnprintf(vaBuffer.data(), VA_BUFFER_SIZE, format, vaList);
    va_end(vaList);

    std::lock_guard<std::mutex> logGuard(logLock);
    const fslib::Path logPath{PATH_JKSV_LOG};
    fslib::File logFile(logPath, FsOpenMode_Append);
    if (logFile.is_open() && logFile.get_size() >= SIZE_LOG_LIMIT)
    {
        logFile.close();
        logFile.open(logPath, FsOpenMode_Create | FsOpenMode_Write);
    }

    if (!logFile.is_open()) { return; }
    logFile << vaBuffer.data() << "\n";
    logFile.flush();
}

#include "system/Task.hpp"
#include <cstdarg>

namespace
{
    /// @brief Size of buffer for formatting the status string.
    constexpr size_t VA_BUFFER_SIZE = 0x1000;
} // namespace

sys::Task::~Task()
{
    m_thread.join();
}

bool sys::Task::is_running() const
{
    return m_isRunning;
}

void sys::Task::finished()
{
    m_isRunning = false;
}

void sys::Task::set_status(const char *format, ...)
{
    char vaBuffer[VA_BUFFER_SIZE] = {0};

    std::va_list vaList;
    va_start(vaList, format);
    vsnprintf(vaBuffer, VA_BUFFER_SIZE, format, vaList);
    va_end(vaList);

    std::scoped_lock<std::mutex> statusLock(m_statusLock);
    m_status = vaBuffer;
}

std::string sys::Task::get_status()
{
    std::scoped_lock<std::mutex> StatusLock(m_statusLock);
    return m_status;
}

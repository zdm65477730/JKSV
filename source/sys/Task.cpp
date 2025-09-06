#include "sys/Task.hpp"

#include "logging/logger.hpp"

sys::Task::~Task() { m_thread.join(); }

bool sys::Task::is_running() const noexcept { return m_isRunning; }

void sys::Task::complete() noexcept { m_isRunning = false; }

void sys::Task::set_status(std::string_view status)
{
    std::lock_guard statusGuard{m_statusLock};
    m_status = status;
}

std::string sys::Task::get_status() noexcept
{
    std::lock_guard statusGuard{m_statusLock};
    return m_status;
}

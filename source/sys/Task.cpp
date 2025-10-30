#include "sys/Task.hpp"

#include "logging/logger.hpp"

//                      ---- Construction ----

sys::Task::Task()
    : m_isRunning(true) {};

sys::Task::Task(sys::threadpool::JobFunction function, sys::Task::TaskData taskData)
    : Task()
{
    taskData->task = this;
    sys::threadpool::push_job(function, taskData);
}

//                      ---- Public functions ----

bool sys::Task::is_running() const noexcept { return m_isRunning; }

void sys::Task::complete() noexcept { m_isRunning = false; }

void sys::Task::set_status(std::string_view status)
{
    std::lock_guard statusGuard{m_statusLock};
    m_status = status;
}

void sys::Task::set_status(std::string &status)
{
    std::lock_guard statusGuard{m_statusLock};
    m_status = std::move(status);
}

std::string sys::Task::get_status() noexcept
{
    std::lock_guard statusGuard{m_statusLock};
    return m_status;
}

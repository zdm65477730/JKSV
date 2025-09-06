#include "sys/ProgressTask.hpp"

void sys::ProgressTask::reset(double goal) noexcept
{
    m_current = 0;
    m_goal    = goal;
}

void sys::ProgressTask::update_current(double current) noexcept { m_current = current; }

void sys::ProgressTask::increase_current(double amount) noexcept { m_current += amount; }

double sys::ProgressTask::get_goal() const noexcept { return m_goal; }

double sys::ProgressTask::get_progress() const noexcept
{
    // Reminder: Never divide by zero. It ends badly every time!
    return m_goal > 0 ? m_current / m_goal : 0;
}

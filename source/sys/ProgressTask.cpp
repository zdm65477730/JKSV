#include "sys/ProgressTask.hpp"

void sys::ProgressTask::reset(double goal)
{
    m_current = 0;
    m_goal    = goal;
}

void sys::ProgressTask::update_current(double current) { m_current = current; }

void sys::ProgressTask::increase_current(double amount) { m_current += amount; }

double sys::ProgressTask::get_goal() const { return m_goal; }

double sys::ProgressTask::get_progress() const
{
    // Reminder: Never divide by zero. It ends badly every time!
    return m_goal > 0 ? m_current / m_goal : 0;
}

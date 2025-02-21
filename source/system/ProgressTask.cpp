#include "system/ProgressTask.hpp"

void sys::ProgressTask::reset(double goal)
{
    m_current = 0;
    m_goal = goal;
}

void sys::ProgressTask::update_current(double current)
{
    m_current = current;
}

double sys::ProgressTask::get_goal(void) const
{
    return m_goal;
}

double sys::ProgressTask::get_current(void) const
{
    return m_current / m_goal;
}

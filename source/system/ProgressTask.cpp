#include "system/ProgressTask.hpp"

void sys::ProgressTask::reset(double goal)
{
    m_current = 0;
    m_goal = goal;
}

void sys::ProgressTask::updateCurrent(double current)
{
    m_current = current;
}

double sys::ProgressTask::getGoal(void) const
{
    return m_goal;
}

double sys::ProgressTask::getCurrent(void) const
{
    return m_current / m_goal;
}

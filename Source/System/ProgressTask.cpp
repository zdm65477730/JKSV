#include "System/ProgressTask.hpp"

void System::ProgressTask::Reset(double Goal)
{
    m_Current = 0;
    m_Goal = Goal;
}

void System::ProgressTask::UpdateCurrent(double Current)
{
    m_Current = Current;
}

double System::ProgressTask::GetGoal(void) const
{
    return m_Goal;
}

double System::ProgressTask::GetCurrentProgress(void) const
{
    return m_Current / m_Goal;
}

#include "System/Timer.hpp"

System::Timer::Timer(uint64_t TriggerTicks) : m_StartingTicks(SDL_GetTicks64()), m_TriggerTicks(TriggerTicks) {};

bool System::Timer::IsTriggered(void)
{
    uint64_t CurrentTicks = SDL_GetTicks64();

    // Nope
    if (CurrentTicks - m_StartingTicks < m_TriggerTicks)
    {
        return false;
    }
    // Reset starting ticks.
    m_StartingTicks = CurrentTicks;
    // Trigger me timbers~
    return true;
}

void System::Timer::Restart(void)
{
    m_StartingTicks = SDL_GetTicks64();
}

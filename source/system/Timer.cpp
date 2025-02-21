#include "system/Timer.hpp"

sys::Timer::Timer(uint64_t triggerTicks)
{
    Timer::start(triggerTicks);
}

sys::Timer &sys::Timer::operator=(const sys::Timer &timer)
{
    m_startingTicks = timer.m_startingTicks;
    m_triggerTicks = timer.m_triggerTicks;
    return *this;
}

void sys::Timer::start(uint64_t triggerTicks)
{
    m_startingTicks = SDL_GetTicks64();
    m_triggerTicks = triggerTicks;
}

bool sys::Timer::is_triggered(void)
{
    uint64_t currentTicks = SDL_GetTicks64();

    // Nope
    if (currentTicks - m_startingTicks < m_triggerTicks)
    {
        return false;
    }
    // Reset starting ticks.
    m_startingTicks = currentTicks;
    // Trigger me timbers~
    return true;
}

void sys::Timer::restart(void)
{
    m_startingTicks = SDL_GetTicks64();
}

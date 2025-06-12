#include "system/Timer.hpp"
#include <SDL2/SDL.h>

#include "logger.hpp"

sys::Timer::Timer(uint64_t triggerTicks)
{
    Timer::start(triggerTicks);
}

void sys::Timer::start(uint64_t triggerTicks)
{
    // Start by recording trigger ticks.
    m_triggerTicks = triggerTicks;

    // Get the current tick count.
    m_startingTicks = SDL_GetTicks64();
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

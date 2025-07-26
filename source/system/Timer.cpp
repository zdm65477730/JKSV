#include "system/Timer.hpp"

#include "logger.hpp"

#include <SDL2/SDL.h>

sys::Timer::Timer(uint64_t triggerTicks) { Timer::start(triggerTicks); }

void sys::Timer::start(uint64_t triggerTicks)
{
    // Start by recording trigger ticks.
    m_triggerTicks = triggerTicks;

    // Get the current tick count.
    m_startingTicks = SDL_GetTicks64();
}

bool sys::Timer::is_triggered()
{
    const uint64_t currentTicks = SDL_GetTicks64();
    if (currentTicks - m_startingTicks < m_triggerTicks) { return false; }

    m_startingTicks = currentTicks;
    // Trigger me timbers~
    return true;
}

void sys::Timer::restart() { m_startingTicks = SDL_GetTicks64(); }

#include "sys/Timer.hpp"

#include "logging/logger.hpp"

#include <SDL2/SDL.h>

sys::Timer::Timer(uint64_t triggerTicks) { Timer::start(triggerTicks); }

void sys::Timer::start(uint64_t triggerTicks)
{
    m_triggerTicks  = triggerTicks;
    m_startingTicks = SDL_GetTicks64();
}

bool sys::Timer::is_triggered()
{
    const uint64_t currentTicks = SDL_GetTicks64();
    const bool started          = m_startingTicks != 0 && m_triggerTicks != 0;
    const bool triggered        = started && (currentTicks - m_startingTicks) >= m_triggerTicks;
    if (!triggered) { return false; }

    m_startingTicks = currentTicks;
    return true;
}

void sys::Timer::restart() { m_startingTicks = SDL_GetTicks64(); }

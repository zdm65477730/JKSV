#pragma once
#include <SDL2/SDL.h>

namespace System
{
    class Timer
    {
        public:
            Timer(uint64_t TriggerTicks);

            // Returns if timer was triggered. Automatically restarts time.
            bool IsTriggered(void);
            // Manually restarts timer.
            void Restart(void);

        private:
            // Beginning ticks.
            uint64_t m_StartingTicks;
            // How many ticks to trigger the timer.
            uint64_t m_TriggerTicks;
    };
} // namespace System

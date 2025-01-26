#pragma once
#include <SDL2/SDL.h>

// Apparently system is used already?
namespace sys
{
    /// @brief Class that uses SDL ticks to time things.
    class Timer
    {
        public:
            /// @brief Default constructor.
            Timer(void) = default;

            /// @brief Constructs a new timer.
            /// @param triggerTicks Number of ticks the timer is triggered at.
            Timer(uint64_t triggerTicks);

            /// @brief Copy operator.
            /// @param timer Timer to copy.
            /// @return Reference to copied timer.
            Timer &operator=(const Timer &timer);

            /// @brief Starts the timer.
            /// @param triggerTicks Number of ticks to trigger at.
            void start(uint64_t triggerTicks);

            /// @brief Updates and returns if the timer was triggered.
            /// @return True if timer is triggered. False if it isn't.
            bool isTriggered(void);

            /// @brief Forces the timer to restart.
            void restart(void);

        private:
            // Beginning ticks.
            uint64_t m_startingTicks;
            // How many ticks to trigger the timer.
            uint64_t m_triggerTicks;
    };
} // namespace sys

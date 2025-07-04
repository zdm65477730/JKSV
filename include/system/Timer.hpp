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
            Timer() = default;

            /// @brief Constructs a new timer.
            /// @param triggerTicks Number of ticks the timer is triggered at.
            Timer(uint64_t triggerTicks);

            /// @brief Starts the timer.
            /// @param triggerTicks Number of ticks to trigger at.
            void start(uint64_t triggerTicks);

            /// @brief Updates and returns if the timer was triggered.
            /// @return True if timer is triggered. False if it isn't.
            bool is_triggered();

            /// @brief Forces the timer to restart.
            void restart();

        private:
            /// @brief Tick count when the timer starts.
            uint64_t m_startingTicks;

            /// @brief Number of ticks to trigger on.
            uint64_t m_triggerTicks;
    };
} // namespace sys

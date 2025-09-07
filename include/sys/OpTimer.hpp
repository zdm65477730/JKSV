#pragma once
#include <chrono>
#include <source_location>

namespace sys
{
    class OpTimer final
    {
        public:
            /// @brief Starts the operation timer.
            OpTimer(const std::source_location &location = std::source_location::current()) noexcept;

            /// @brief Ends the timer and
            ~OpTimer() noexcept;

        private:
            // Stores a reference to the location passed.
            const std::source_location m_location;

            /// @brief Stores the time the timer was started.
            std::chrono::high_resolution_clock::time_point m_begin{};

            /// @brief Returns a string_view containing just the function name. No return type.
            std::string_view get_function_name() const noexcept;
    };
}
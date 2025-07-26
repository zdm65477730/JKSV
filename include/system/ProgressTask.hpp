#pragma once
#include "system/Task.hpp"

namespace sys
{
    /// @brief Derived class of Task that has methods for tracking progress.
    class ProgressTask final : public sys::Task
    {
        public:
            /// @brief Contstructs a new ProgressTask
            /// @param function Function for thread to execute.
            /// @param args Arguments to forward to the thread function.
            /// @note All functions passed to this must follow this signature: void function(sys::ProgressTask *, <arguments>)
            template <typename... Args>
            ProgressTask(void (*function)(sys::ProgressTask *, Args...), Args... args)
                : sys::Task(function, this, std::forward<Args>(args)...){};

            /// @brief Resets the progress and sets a new goal.
            /// @param goal The goal we all strive for.
            void reset(double goal);

            /// @brief Updates the current progress.
            /// @param current The current progress value.
            void update_current(double current);

            /// @brief Increases the current progress by a set amount.
            void increase_current(double amount);

            /// @brief Returns the goal value.
            /// @return Goal
            double get_goal() const;

            /// @brief Returns the current progress.
            /// @return Current progress.
            double get_progress() const;

        private:
            // Current value and goal
            double m_current{};
            double m_goal{};
    };
} // namespace sys

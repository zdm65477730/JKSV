#pragma once
#include "appstates/BaseTask.hpp"
#include "system/ProgressTask.hpp"
#include <string>
#include <switch.h>

/// @brief State that shows progress of a task.
class ProgressState final : public BaseTask
{
    public:
        /// @brief Constructs a new ProgressState.
        /// @param function Function for the task to run.
        /// @param args Variadic arguments to be forwarded to the function passed.
        /// @note All functions passed to this must follow this signature: void function(sys::ProgressTask *, <arguments>)
        template <typename... Args>
        ProgressState(void (*function)(sys::ProgressTask *, Args...), Args... args)
            : BaseTask(), m_task(function, std::forward<Args>(args)...){};

        /// @brief Required destructor.
        ~ProgressState() {};

        /// @brief Checks if the thread is finished and deactivates this state.
        void update() override;

        /// @brief Renders the current progress to screen.
        void render() override;

    private:
        /// @brief Underlying task that has extra methods for tracking the progress of a task.
        sys::ProgressTask m_task;

        /// @brief Progress which is saved as a rounded whole number.
        size_t m_progress = 0;

        /// @brief Width of the green bar in pixels.
        size_t m_progressBarWidth = 0;

        /// @brief X coordinate of the percentage string.
        int m_percentageX = 0;

        /// @brief Percentage as a string for printing to screen.
        std::string m_percentageString;
};

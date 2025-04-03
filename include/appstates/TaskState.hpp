#pragma once
#include "appstates/AppState.hpp"
#include "system/Task.hpp"
#include <switch.h>

/// @brief State that spawns a task and allows updates to be printed to screen.
class TaskState : public AppState
{
    public:
        /// @brief Constructs and spawns a new TaskState.
        /// @param function Function to run in the thread.
        /// @param args Variadic templated arguments to forward.
        /// @note All functions passed must follow this signature: void function(sys::Task *, <arguments>)
        template <typename... Args>
        TaskState(void (*function)(sys::Task *, Args...), Args... args)
            : AppState(false), m_task(function, std::forward<Args>(args)...){};

        /// @brief Required destructor.
        ~TaskState() {};

        /// @brief Runs update routine. Waits for thread function to signal finish and deactivates.
        void update(void) override;

        /// @brief Run render routine. Prints m_task's status string to screen, basically.
        /// @param
        void render(void) override;

    private:
        /// @brief Underlying task.
        sys::Task m_task;
};

#pragma once
#include "StateManager.hpp"
#include "appstates/BaseTask.hpp"
#include "sys/sys.hpp"

#include <switch.h>

/// @brief State that spawns a task and allows updates to be printed to screen.
class TaskState final : public BaseTask
{
    public:
        /// @brief Constructs and spawns a new TaskState.
        /// @param function Function to run in the thread.
        /// @param args Variadic templated arguments to forward.
        /// @note All functions passed must follow this signature: void function(sys::Task *, <arguments>)
        template <typename... Args>
        TaskState(void (*function)(sys::Task *, Args...), Args... args)
            : BaseTask()
        {
            m_task = std::make_unique<sys::Task>(function, std::forward<Args>(args)...);
        }

        /// @brief Required destructor.
        ~TaskState() {};

        /// @brief Creates and returns a new TaskState.
        template <typename... Args>
        static inline std::shared_ptr<TaskState> create(void (*function)(sys::Task *, Args...), Args... args)
        {
            return std::make_shared<TaskState>(function, std::forward<Args>(args)...);
        }

        /// @brief Creates, pushes, then returns and new TaskState.
        template <typename... Args>
        static inline std::shared_ptr<TaskState> create_and_push(void (*function)(sys::Task *, Args...), Args... args)
        {
            auto newState = TaskState::create(function, std::forward<Args>(args)...);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Runs update routine. Waits for thread function to signal finish and deactivates.
        void update() override;

        /// @brief Run render routine. Prints m_task's status string to screen, basically.
        /// @param
        void render() override;

    private:
        /// @brief Performs some operations and marks the state for deletion.
        void deactivate_state();
};

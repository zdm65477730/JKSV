#pragma once
#include "StateManager.hpp"
#include "appstates/BaseTask.hpp"
#include "sys/sys.hpp"
#include "ui/ui.hpp"

#include <functional>
#include <memory>
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
            : BaseTask()
        {
            ProgressState::initialize_static_members();
            m_task = std::make_unique<sys::ProgressTask>(function, std::forward<Args>(args)...);
        }

        /// @brief Required destructor.
        ~ProgressState();

        /// @brief Creates and returns a new progress state.
        template <typename... Args>
        static inline std::shared_ptr<ProgressState> create(void (*function)(sys::ProgressTask *, Args...), Args... args)
        {
            return std::make_shared<ProgressState>(function, std::forward<Args>(args)...);
        }

        /// @brief Creates, pushes, then returns a new ProgressState.
        template <typename... Args>
        static inline std::shared_ptr<ProgressState> create_and_push(void (*function)(sys::ProgressTask *, Args...),
                                                                     Args... args)
        {
            auto newState = ProgressState::create(function, std::forward<Args>(args)...);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Checks if the thread is finished and deactivates this state.
        void update() override;

        /// @brief Renders the current progress to screen.
        void render() override;

    private:
        /// @brief Progress which is saved as a rounded whole number.
        size_t m_progress{};

        /// @brief Width of the green bar in pixels.
        size_t m_progressBarWidth{};

        /// @brief X coordinate of the percentage string.
        int m_percentageX{};

        /// @brief Percentage as a string for printing to screen.
        std::string m_percentageString{};

        /// @brief This is the dialog box everything is rendered to.
        static inline std::shared_ptr<ui::DialogBox> sm_dialog{};

        void initialize_static_members();
};

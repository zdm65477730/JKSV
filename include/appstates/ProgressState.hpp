#pragma once
#include "StateManager.hpp"
#include "appstates/BaseTask.hpp"
#include "appstates/FadeState.hpp"
#include "graphics/colors.hpp"
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
        ProgressState(sys::threadpool::JobFunction function, sys::Task::TaskData taskData);

        /// @brief Creates and returns a new ProgressState
        static inline std::shared_ptr<ProgressState> create(sys::threadpool::JobFunction function, sys::Task::TaskData taskData)
        {
            return std::make_shared<ProgressState>(function, taskData);
        }

        /// @brief Creates, pushes, then returns a new ProgressState.
        static inline std::shared_ptr<ProgressState> create_and_push(sys::threadpool::JobFunction function,
                                                                     sys::Task::TaskData taskData)
        {
            auto newState = ProgressState::create(function, taskData);
            StateManager::push_state(newState);
            return newState;
        }

        static inline std::shared_ptr<ProgressState> create_push_fade(sys::threadpool::JobFunction function,
                                                                      sys::Task::TaskData taskData)
        {
            auto newState = ProgressState::create(function, taskData);
            FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_BEGIN, colors::ALPHA_FADE_END, newState);
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

        /// @brief This is rendered over the edges of the bar to give it a slightly rounded look.
        static inline sdl::SharedTexture sm_barEdges{};

        /// @brief Initializes the shared dialog box.
        void initialize_static_members();

        /// @brief Performs some cleanup and marks the state for purging.
        void deactivate_state();
};

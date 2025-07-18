#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/TaskState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "system/Task.hpp"
#include "ui/render_functions.hpp"

#include <memory>
#include <string>
#include <switch.h>

namespace
{
    // This is the base position on the screen used to center the Yes [A](...) text.
    constexpr int YES_X_CENTER_COORDINATE = 460;
} // namespace

/// @brief Templated class to create confirmation dialogs.
/// @tparam TaskType The type of task spawned on confirmation. Ex: Task, ProgressTask
/// @tparam StateType The state type spawned on confirmation. Ex: TaskState, ProgressState
/// @tparam StructType The type of struct passed to the state on confirmation.
template <typename TaskType, typename StateType, typename StructType>
class ConfirmState final : public BaseState
{
    public:
        /// @brief All functions passed to this state need to follow this signature: void function(<TaskType> *,
        /// std::shared_ptr<<StructType>>)
        using TaskFunction = void (*)(TaskType *, std::shared_ptr<StructType>);

        /// @brief Constructor for new ConfirmState.
        /// @param queryString The string displayed.
        /// @param holdRequired Whether or not confirmation requires holding A for three seconds.
        /// @param function Function executed on confirmation.
        /// @param dataStruct shared_ptr<StructType> that is passed to function. I tried templating this and it was a nightmare.
        ConfirmState(std::string_view queryString,
                     bool holdRequired,
                     TaskFunction function,
                     std::shared_ptr<StructType> dataStruct)
            : BaseState(false)
            , m_queryString(queryString)
            , m_yesString(strings::get_by_name(strings::names::YES_NO, 0))
            , m_hold(holdRequired)
            , m_function(function)
            , m_dataStruct(dataStruct)
        {
            // This is to make centering the Yes [A] string more accurate.
            m_yesX = YES_X_CENTER_COORDINATE - (sdl::text::get_width(22, m_yesString.c_str()) / 2);
            m_noX  = 820 - (sdl::text::get_width(22, strings::get_by_name(strings::names::YES_NO, 1)) / 2);
        }

        /// @brief Required even if it does nothing.
        ~ConfirmState() {};

        /// @brief Just updates the ConfirmState.
        void update() override
        {
            // This is to guard against the dialog being triggered right away. To do: Maybe figure out a better way to
            // accomplish this?
            if (input::button_pressed(HidNpadButton_A) && !m_triggerGuard) { m_triggerGuard = true; }

            if (m_triggerGuard && input::button_pressed(HidNpadButton_A) && !m_hold)
            {
                BaseState::deactivate();

                auto newState = std::make_shared<StateType>(m_function, m_dataStruct);

                StateManager::push_state(newState);
            }
            else if (m_triggerGuard && input::button_pressed(HidNpadButton_A) && m_hold)
            {
                // Get the starting tick count and change the Yes string to the first holding string.
                m_startingTickCount = SDL_GetTicks64();
                m_yesString         = strings::get_by_name(strings::names::HOLDING_STRINGS, 0);
            }
            else if (m_triggerGuard && input::button_held(HidNpadButton_A) && m_hold)
            {
                uint64_t TickCount = SDL_GetTicks64() - m_startingTickCount;

                // If the TickCount is >= 3 seconds, confirmed. Else, just change the string so we can see we're not holding for
                // nothing?
                if (TickCount >= 3000)
                {
                    BaseState::deactivate();

                    auto newState = std::make_shared<StateType>(m_function, m_dataStruct);
                }
                else if (TickCount >= 2000)
                {
                    m_yesString = strings::get_by_name(strings::names::HOLDING_STRINGS, 2);
                    m_yesX      = YES_X_CENTER_COORDINATE - (sdl::text::get_width(22, m_yesString.c_str()) / 2);
                }
                else if (TickCount >= 1000)
                {
                    m_yesString = strings::get_by_name(strings::names::HOLDING_STRINGS, 1);
                    m_yesX      = YES_X_CENTER_COORDINATE - (sdl::text::get_width(22, m_yesString.c_str()) / 2);
                }
            }
            else if (input::button_released(HidNpadButton_A))
            {
                m_yesString = strings::get_by_name(strings::names::YES_NO, 0);
                m_yesX      = YES_X_CENTER_COORDINATE - (sdl::text::get_width(22, m_yesString.c_str()) / 2);
            }
            else if (input::button_pressed(HidNpadButton_B))
            {
                // Just deactivate and don't do anything.
                BaseState::deactivate();
            }
        }

        /// @brief Renders the state to screen.
        void render() override
        {
            // Dim background
            sdl::render_rect_fill(NULL, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
            // Render dialog
            ui::render_dialog_box(NULL, 280, 262, 720, 256);
            // Text
            sdl::text::render(NULL, 312, 288, 18, 656, colors::WHITE, m_queryString.c_str());
            // Fake buttons. Maybe real later.
            sdl::render_line(NULL, 280, 454, 999, 454, colors::WHITE);
            sdl::render_line(NULL, 640, 454, 640, 517, colors::WHITE);
            // To do: Position this better. Currently brought over from old code.
            sdl::text::render(NULL, m_yesX, 476, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_yesString.c_str());
            sdl::text::render(NULL,
                              m_noX,
                              476,
                              22,
                              sdl::text::NO_TEXT_WRAP,
                              colors::WHITE,
                              strings::get_by_name(strings::names::YES_NO, 1));
        }

    private:
        /// @brief String displayed
        std::string m_queryString{};

        /// @brief Yes or [X] [A]
        std::string m_yesString{};

        /// @brief X coordinate to render the Yes [A]
        int m_yesX{};

        /// @brief Position of No.
        int m_noX{};

        /// @brief This is to prevent the dialog from triggering immediately.
        bool m_triggerGuard{};

        /// @brief Whether or not holding [A] to confirm is required.
        bool m_hold{};

        /// @brief Keep track of the ticks/time needed to confirm.
        uint64_t m_startingTickCount{};

        /// @brief Function to execute if action is confirmed.
        TaskFunction m_function{};

        /// @brief Pointer to data struct passed to ^
        std::shared_ptr<StructType> m_dataStruct{};
};

#pragma once
#include "JKSV.hpp"
#include "appstates/AppState.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/TaskState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "system/Task.hpp"
#include "ui/renderFunctions.hpp"
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
class ConfirmState : public AppState
{
    public:
        /// @brief All functions passed to this state need to follow this signature: void function(<TaskType> *, std::shared_ptr<<StructType>>)
        using TaskFunction = void (*)(TaskType *, std::shared_ptr<StructType>);

        /// @brief Constructor for new ConfirmState.
        /// @param queryString The string displayed.
        /// @param holdRequired Whether or not confirmation requires holding A for three seconds.
        /// @param function Function executed on confirmation.
        /// @param dataStruct shared_ptr<StructType> that is passed to function. I tried templating this and it was a nightmare.
        ConfirmState(std::string_view queryString, bool holdRequired, TaskFunction function, std::shared_ptr<StructType> dataStruct)
            : AppState(false), m_queryString(queryString.data()), m_yesString(strings::getByName(strings::names::YES_NO, 0)),
              m_hold(holdRequired), m_function(function), m_dataStruct(dataStruct)
        {
            // This is to make centering the Yes [A] string more accurate.
            m_yesX = YES_X_CENTER_COORDINATE - (sdl::text::getWidth(22, m_yesString.c_str()) / 2);
            m_noX = 820 - (sdl::text::getWidth(22, strings::getByName(strings::names::YES_NO, 1)) / 2);
        }

        /// @brief Required even if it does nothing.
        ~ConfirmState() {};

        /// @brief Just updates the ConfirmState.
        void update(void)
        {
            if (input::buttonPressed(HidNpadButton_A) && !m_hold)
            {
                AppState::deactivate();
                JKSV::pushState(std::make_shared<StateType>(m_function, m_dataStruct));
            }
            else if (input::buttonPressed(HidNpadButton_A) && m_hold)
            {
                // Get the starting tick count and change the Yes string to the first holding string.
                m_startingTickCount = SDL_GetTicks64();
                m_yesString = strings::getByName(strings::names::HOLDING_STRINGS, 0);
            }
            else if (input::buttonHeld(HidNpadButton_A) && m_hold)
            {
                uint64_t TickCount = SDL_GetTicks64() - m_startingTickCount;

                // If the TickCount is >= 3 seconds, confirmed. Else, just change the string so we can see we're not holding for nothing?
                if (TickCount >= 3000)
                {
                    AppState::deactivate();
                    JKSV::pushState(std::make_shared<StateType>(m_function, m_dataStruct));
                }
                else if (TickCount >= 2000)
                {
                    m_yesString = strings::getByName(strings::names::HOLDING_STRINGS, 2);
                    m_yesX = YES_X_CENTER_COORDINATE - (sdl::text::getWidth(22, m_yesString.c_str()) / 2);
                }
                else if (TickCount >= 1000)
                {
                    m_yesString = strings::getByName(strings::names::HOLDING_STRINGS, 1);
                    m_yesX = YES_X_CENTER_COORDINATE - (sdl::text::getWidth(22, m_yesString.c_str()) / 2);
                }
            }
            else if (input::buttonReleased(HidNpadButton_A))
            {
                m_yesString = strings::getByName(strings::names::YES_NO, 0);
                m_yesX = YES_X_CENTER_COORDINATE - (sdl::text::getWidth(22, m_yesString.c_str()) / 2);
            }
            else if (input::buttonPressed(HidNpadButton_B))
            {
                // Just deactivate and don't do anything.
                AppState::deactivate();
            }
        }

        /// @brief Renders the state to screen.
        void render(void)
        {
            // Dim background
            sdl::renderRectFill(NULL, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
            // Render dialog
            ui::renderDialogBox(NULL, 280, 262, 720, 256);
            // Text
            sdl::text::render(NULL, 312, 288, 18, 656, colors::WHITE, m_queryString.c_str());
            // Fake buttons. Maybe real later.
            sdl::renderLine(NULL, 280, 454, 999, 454, colors::WHITE);
            sdl::renderLine(NULL, 640, 454, 640, 517, colors::WHITE);
            // To do: Position this better. Currently brought over from old code.
            sdl::text::render(NULL, m_yesX, 476, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_yesString.c_str());
            sdl::text::render(NULL, m_noX, 476, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, strings::getByName(strings::names::YES_NO, 1));
        }

    private:
        // Query string
        std::string m_queryString;
        // Yes string.
        std::string m_yesString;
        // X coordinate to render the Yes [A] string to,
        int m_yesX = 0;
        // X coordinate to render the No [B] string to.
        int m_noX = 0;
        // Whether or not holding is required to confirm.
        bool m_hold;
        // For tick counting/holding
        uint64_t m_startingTickCount = 0;
        // Function
        TaskFunction m_function;
        // Shared ptr to data to send to confirmation function.
        std::shared_ptr<StructType> m_dataStruct;
};

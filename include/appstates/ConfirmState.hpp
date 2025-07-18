#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/TaskState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "logger.hpp"
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
    constexpr int COORD_YES_X = 460;
    // ^ but for no.
    constexpr int COORD_NO_X = 820;
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
            , m_yesText(strings::get_by_name(strings::names::YES_NO, 0))
            , m_noText(strings::get_by_name(strings::names::YES_NO, 1))
            , m_holdRequired(holdRequired)
            , m_function(function)
            , m_dataStruct(dataStruct)
        {
            const int yesWidth = sdl::text::get_width(22, m_yesText);
            const int noWidth  = sdl::text::get_width(22, m_noText);

            // This stays the same from here on out.
            m_noX = COORD_NO_X - (noWidth / 2);
            ConfirmState::center_yes();

            // Gonna loop to do this.
            for (int i = 0; i < 3; i++) { m_holdText[i] = strings::get_by_name(strings::names::HOLDING_STRINGS, i); }
        }

        /// @brief Required even if it does nothing.
        ~ConfirmState() {};

        /// @brief Just updates the ConfirmState.
        void update() override
        {
            const bool aPressed  = input::button_pressed(HidNpadButton_A);
            const bool bPressed  = input::button_pressed(HidNpadButton_B);
            const bool aHeld     = input::button_held(HidNpadButton_A);
            const bool aReleased = input::button_released(HidNpadButton_A);

            m_triggerGuard           = m_triggerGuard || (aPressed && !m_triggerGuard);
            const bool noHoldTrigger = m_triggerGuard && aPressed && !m_holdRequired;
            const bool holdTriggered = m_triggerGuard && aPressed && m_holdRequired;
            const bool holdSustained = m_triggerGuard && aHeld && m_holdRequired;

            if (noHoldTrigger)
            {
                ConfirmState::create_push_state();
                BaseState::deactivate();
            }
            else if (holdTriggered)
            {
                m_startingTickCount = SDL_GetTicks64();
                m_yesText           = m_holdText[0];
            }
            else if (holdSustained)
            {
                const uint64_t totalTicks = SDL_GetTicks64() - m_startingTickCount;
                const bool threeSeconds   = totalTicks >= 3000;
                const bool twoSeconds     = totalTicks >= 2000;
                const bool oneSecond      = totalTicks >= 1000;

                if (threeSeconds)
                {
                    ConfirmState::create_push_state();
                    BaseState::deactivate();
                }
                else if (twoSeconds)
                {
                    m_yesText = m_holdText[2];
                    ConfirmState::center_yes();
                }
                else if (oneSecond)
                {
                    m_yesText = m_holdText[1];
                    ConfirmState::center_yes();
                }
            }
            else if (aReleased)
            {
                m_yesText = strings::get_by_name(strings::names::YES_NO, 0);
                ConfirmState::center_yes();
            }
            else if (bPressed) { BaseState::deactivate(); }
        }

        /// @brief Renders the state to screen.
        void render() override
        {
            sdl::render_rect_fill(NULL, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
            ui::render_dialog_box(NULL, 280, 262, 720, 256);

            sdl::text::render(NULL, 312, 288, 18, 656, colors::WHITE, m_queryString.c_str());

            sdl::render_line(NULL, 280, 454, 999, 454, colors::WHITE);
            sdl::render_line(NULL, 640, 454, 640, 517, colors::WHITE);

            sdl::text::render(NULL, m_yesX, 476, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_yesText);
            sdl::text::render(NULL, m_noX, 476, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_noText);
        }

    private:
        /// @brief String displayed
        const std::string m_queryString{};

        /// @brief These are pointers to the strings used to avoid call strings::get_by_name so much.
        const char *m_yesText{}, *m_noText{};

        /// @brief X coordinate to render the Yes No
        int m_yesX{}, m_noX{};

        /// @brief This is to prevent the dialog from triggering immediately.
        bool m_triggerGuard{};

        /// @brief Whether or not holding [A] to confirm is required.
        const bool m_holdRequired{};

        /// @brief These are pointers to the holding strings.
        const char *m_holdText[3];

        /// @brief Keep track of the ticks/time needed to confirm.
        uint64_t m_startingTickCount{};

        /// @brief Function to execute if action is confirmed.
        const TaskFunction m_function{};

        /// @brief Pointer to data struct passed to ^
        const std::shared_ptr<StructType> m_dataStruct{};

        void create_push_state()
        {
            auto newState = std::make_shared<StateType>(m_function, m_dataStruct);
            StateManager::push_state(newState);
        }

        // This just centers the Yes or holding text.
        void center_yes()
        {
            const size_t yesWidth = sdl::text::get_width(22, m_yesText);
            m_yesX                = COORD_YES_X - (yesWidth / 2);
        }
};

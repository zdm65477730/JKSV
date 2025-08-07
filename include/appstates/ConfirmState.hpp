#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "appstates/FadeState.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/TaskState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "sys/sys.hpp"
#include "ui/ui.hpp"

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
        /// @param query The string displayed.
        /// @param holdRequired Whether or not confirmation requires holding A for three seconds.
        /// @param function Function executed on confirmation.
        /// @param dataStruct shared_ptr<StructType> that is passed to function. I tried templating this and it was a nightmare.
        ConfirmState(std::string_view query, bool holdRequired, TaskFunction function, std::shared_ptr<StructType> dataStruct)
            : BaseState{false}
            , m_query(query)
            , m_yesText(strings::get_by_name(strings::names::YES_NO_OK, 0))
            , m_noText(strings::get_by_name(strings::names::YES_NO_OK, 1))
            , m_holdRequired(holdRequired)
            , m_function(function)
            , m_dataStruct(dataStruct)
        {
            ConfirmState::initialize_static_members();
            ConfirmState::center_no();
            ConfirmState::center_yes();
            ConfirmState::load_holding_strings();
        }

        /// @brief Required even if it does nothing.
        ~ConfirmState() {};

        /// @brief Returns a new ConfirmState. See constructor.
        static std::shared_ptr<ConfirmState> create(std::string_view query,
                                                    bool holdRequired,
                                                    TaskFunction function,
                                                    std::shared_ptr<StructType> dataStruct)
        {
            return std::make_shared<ConfirmState>(query, holdRequired, function, dataStruct);
        }

        /// @brief Creates and returns a new ConfirmState and pushes it.
        static std::shared_ptr<ConfirmState> create_and_push(std::string_view query,
                                                             bool holdRequired,
                                                             TaskFunction function,
                                                             std::shared_ptr<StructType> dataStruct)
        {
            // I'm gonna use a sneaky trick here. This shouldn't do this because it's confusing.
            auto newState = create(query, holdRequired, function, dataStruct);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Same as above but with a fade transition pushed  in between.
        static std::shared_ptr<ConfirmState> create_push_fade(std::string_view query,
                                                              bool holdRequired,
                                                              TaskFunction function,
                                                              std::shared_ptr<StructType> dataStruct)
        {
            auto newState = ConfirmState::create(query, holdRequired, function, dataStruct);
            FadeState::create_and_push(colors::DIM_BACKGROUND, 0x00, 0x88, newState);
            return newState;
        }

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

            if (noHoldTrigger) { ConfirmState::confirmed(); }
            else if (holdTriggered) { ConfirmState::hold_triggered(); }
            else if (holdSustained) { ConfirmState::hold_sustained(); }
            else if (aReleased) { ConfirmState::hold_released(); }
            else if (bPressed) { ConfirmState::cancelled(); }
        }

        /// @brief Renders the state to screen.
        void render() override
        {
            const bool hasFocus = BaseState::has_focus();

            sdl::render_rect_fill(sdl::Texture::Null, 0, 0, 1280, 720, colors::DIM_BACKGROUND);

            sm_dialog->render(sdl::Texture::Null, hasFocus);
            sdl::text::render(sdl::Texture::Null, 312, 288, 20, 656, colors::WHITE, m_query);

            sdl::render_line(sdl::Texture::Null, 280, 454, 999, 454, colors::DIV_COLOR);
            sdl::render_line(sdl::Texture::Null, 640, 454, 640, 517, colors::DIV_COLOR);

            sdl::text::render(sdl::Texture::Null, m_yesX, 476, 22, sdl::text::NO_WRAP, colors::WHITE, m_yesText);
            sdl::text::render(sdl::Texture::Null, m_noX, 476, 22, sdl::text::NO_WRAP, colors::WHITE, m_noText);
        }

    private:
        /// @brief This stores the query text.
        std::string m_query{};

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

        /// @brief Keeps track of which
        int m_stage{};

        /// @brief Function to execute if action is confirmed.
        const TaskFunction m_function{};

        /// @brief Pointer to data struct passed to ^
        const std::shared_ptr<StructType> m_dataStruct{};

        static inline std::shared_ptr<ui::DialogBox> sm_dialog{};

        void initialize_static_members()
        {
            if (sm_dialog) { return; }

            sm_dialog = ui::DialogBox::create(280, 262, 720, 256);
        }

        // This just centers the Yes or holding text.
        void center_yes()
        {
            const int yesWidth = sdl::text::get_width(22, m_yesText);
            m_yesX             = COORD_YES_X - (yesWidth / 2);
        }

        void center_no()
        {
            const int noWidth = sdl::text::get_width(22, m_noText);
            m_noX             = COORD_NO_X - (noWidth / 2);
        }

        void load_holding_strings()
        {
            for (int i = 0; const char *string = strings::get_by_name(strings::names::HOLDING_STRINGS, i); i++)
            {
                m_holdText[i] = string;
            }
        }

        void confirmed()
        {
            auto newState = std::make_shared<StateType>(m_function, m_dataStruct);
            StateManager::push_state(newState);
            BaseState::deactivate();
        }

        void hold_triggered()
        {
            m_startingTickCount = SDL_GetTicks64();
            ConfirmState::change_holding_text(0);
        }

        void hold_sustained()
        {
            const uint64_t totalTicks = SDL_GetTicks64() - m_startingTickCount;
            const bool threeSeconds   = totalTicks >= 3000;
            const bool twoSeconds     = totalTicks >= 2000;
            const bool oneSecond      = totalTicks >= 1000;

            if (threeSeconds) { ConfirmState::confirmed(); }
            else if (twoSeconds) { ConfirmState::change_holding_text(2); }
            else if (oneSecond) { ConfirmState::change_holding_text(1); }
        }

        void hold_released()
        {
            m_yesText = strings::get_by_name(strings::names::YES_NO_OK, 0);
            ConfirmState::center_yes();
        }

        void cancelled()
        {
            FadeState::create_and_push(colors::DIM_BACKGROUND, 0x88, 0x00, nullptr);
            BaseState::deactivate();
        }

        void change_holding_text(int index)
        {
            m_yesText = m_holdText[index];
            ConfirmState::center_yes();
        }
};

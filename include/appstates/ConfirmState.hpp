#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "appstates/FadeState.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/TaskState.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "logging/logger.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"
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
template <typename TaskType, typename StateType>
class ConfirmState final : public BaseState
{
    public:
        /// @brief Constructor for new ConfirmState.
        /// @param query The string displayed.
        /// @param holdRequired Whether or not confirmation requires holding A for three seconds.
        /// @param function Function executed on confirmation.
        /// @param dataStruct shared_ptr<StructType> that is passed to function. I tried templating this and it was a nightmare.
        ConfirmState(std::string_view query,
                     bool holdRequired,
                     sys::threadpool::JobFunction onConfirm,
                     sys::threadpool::JobFunction onCancel,
                     sys::Task::TaskData taskData)
            : BaseState(false)
            , m_query(query)
            , m_yesText(strings::get_by_name(strings::names::YES_NO_OK, 0))
            , m_noText(strings::get_by_name(strings::names::YES_NO_OK, 1))
            , m_holdRequired(holdRequired)
            , m_transition(280, 229, 32, 32, 280, 229, 720, 256, 4)
            , m_onConfirm(onConfirm)
            , m_onCancel(onCancel)
            , m_taskData(taskData)
        {
            ConfirmState::initialize_static_members();
            ConfirmState::center_no();
            ConfirmState::center_yes();
            ConfirmState::load_holding_strings();
            sm_dialogPop->play();
        }

        /// @brief Returns a new ConfirmState. See constructor.
        static inline std::shared_ptr<ConfirmState> create(std::string_view query,
                                                           bool holdRequired,
                                                           sys::threadpool::JobFunction onConfirm,
                                                           sys::threadpool::JobFunction onCancel,
                                                           sys::Task::TaskData taskData)
        {
            return std::make_shared<ConfirmState>(query, holdRequired, onConfirm, onCancel, taskData);
        }

        /// @brief Creates and returns a new ConfirmState and pushes it.
        static inline std::shared_ptr<ConfirmState> create_and_push(std::string_view query,
                                                                    bool holdRequired,
                                                                    sys::threadpool::JobFunction onConfirm,
                                                                    sys::threadpool::JobFunction onCancel,
                                                                    sys::Task::TaskData taskData)
        {
            // I'm gonna use a sneaky trick here. This shouldn't do this because it's confusing.
            auto newState = create(query, holdRequired, onConfirm, onCancel, taskData);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Same as above but with a fade transition pushed  in between.
        static std::shared_ptr<ConfirmState> create_push_fade(std::string_view query,
                                                              bool holdRequired,
                                                              sys::threadpool::JobFunction onConfirm,
                                                              sys::threadpool::JobFunction onCancel,
                                                              sys::Task::TaskData taskData)
        {
            auto newState = ConfirmState::create(query, holdRequired, onConfirm, onCancel, taskData);
            FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_BEGIN, colors::ALPHA_FADE_END, newState);
            return newState;
        }

        /// @brief Just updates the ConfirmState.
        void update() override
        {
            m_transition.update();
            sm_dialog->set_from_transition(m_transition, true);
            if (!m_transition.in_place()) { return; }

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
            else if (bPressed) { ConfirmState::close_dialog(); }
            else if (m_close && m_transition.in_place()) { ConfirmState::deactivate_state(); }
        }

        /// @brief Renders the state to screen.
        void render() override
        {
            const bool hasFocus = BaseState::has_focus();
            const int y         = m_transition.get_y();

            sdl::render_rect_fill(sdl::Texture::Null, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
            sm_dialog->render(sdl::Texture::Null, hasFocus);
            if (!m_transition.in_place() || m_close) { return; }

            sdl::text::render(sdl::Texture::Null, 312, y + 24, 20, 656, colors::WHITE, m_query);

            sdl::render_line(sdl::Texture::Null, 280, y + 192, 999, y + 192, colors::DIV_COLOR);
            sdl::render_line(sdl::Texture::Null, 640, y + 192, 640, y + 255, colors::DIV_COLOR);

            sdl::text::render(sdl::Texture::Null, m_yesX, y + 214, 22, sdl::text::NO_WRAP, colors::WHITE, m_yesText);
            sdl::text::render(sdl::Texture::Null, m_noX, y + 214, 22, sdl::text::NO_WRAP, colors::WHITE, m_noText);
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

        /// @brief Whether or not the action was confirmed
        bool m_confirmed{};

        /// @brief Whether or not holding [A] to confirm is required.
        const bool m_holdRequired{};

        /// @brief These are pointers to the holding strings.
        const char *m_holdText[3]{};

        /// @brief Keep track of the ticks/time needed to confirm.
        uint64_t m_startingTickCount{};

        /// @brief Keeps track of which stage the confirmation is in.
        int m_stage{};

        /// @brief To make the dialog pop in from bottom to be more inline with the rest of the UI.
        ui::Transition m_transition{};

        // Controls whether or not to close or hide the dialog.
        bool m_close{};

        /// @brief Function to execute if action is confirmed.
        const sys::threadpool::JobFunction m_onConfirm{};

        /// @brief Function to execute if action is cancelled.
        const sys::threadpool::JobFunction m_onCancel{};

        /// @brief Pointer to data struct passed to ^
        const sys::Task::TaskData m_taskData{};

        /// @brief This dialog is shared between all instances.
        static inline std::shared_ptr<ui::DialogBox> sm_dialog{};

        /// @brief This sound is shared.
        static inline sdl::SharedSound sm_dialogPop{};

        void initialize_static_members()
        {
            static constexpr std::string_view POP_SOUND = "ConfirmPop";
            static constexpr const char *POP_PATH       = "romfs:/Sound/ConfirmPop.wav";

            if (sm_dialog && sm_dialogPop) { return; }

            sm_dialog    = ui::DialogBox::create(0, 0, 0, 0);
            sm_dialogPop = sdl::SoundManager::load(POP_SOUND, POP_PATH);
            sm_dialog->set_from_transition(m_transition, true);
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
            m_confirmed = true;
            ConfirmState::close_dialog();
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

        void deactivate_state()
        {
            if (m_confirmed && m_onConfirm) { StateType::create_and_push(m_onConfirm, m_taskData); }
            else if (!m_confirmed && m_onCancel) { StateType::create_and_push(m_onCancel, m_taskData); }
            else
            {
                FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_END, colors::ALPHA_FADE_BEGIN, nullptr);
            }
            BaseState::deactivate();
        }

        void change_holding_text(int index)
        {
            m_yesText = m_holdText[index];
            ConfirmState::center_yes();
        }

        void close_dialog()
        {
            m_close = true;
            m_transition.set_target_width(32);
            m_transition.set_target_height(32);
        }
};

using ConfirmTask     = ConfirmState<sys::Task, TaskState>;
using ConfirmProgress = ConfirmState<sys::ProgressTask, ProgressState>;

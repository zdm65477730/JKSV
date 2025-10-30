#include "appstates/MessageState.hpp"

#include "StateManager.hpp"
#include "appstates/FadeState.hpp"
#include "graphics/colors.hpp"
#include "graphics/screen.hpp"
#include "input.hpp"
#include "strings/strings.hpp"

//                      ---- Construction ----

MessageState::MessageState(std::string_view message)
    : m_message(message)
    , m_transition(0, 0, 32, 32, 0, 0, graphics::SCREEN_HEIGHT, 256, ui::Transition::DEFAULT_THRESHOLD)
{
    MessageState::initialize_static_members();
    sm_dialogPop->play();
}

//                      ---- Public functions ----

void MessageState::update()
{
    m_transition.update();
    sm_dialog->set_from_transition(m_transition, true);
    if (!m_transition.in_place()) { return; }

    // To do: I only use this in one place right now. I'm not sure this guards correctly here?
    const bool aPressed = input::button_pressed(HidNpadButton_A);
    m_triggerGuard      = m_triggerGuard || (aPressed && !m_triggerGuard);
    const bool finished = m_triggerGuard && aPressed;

    if (finished) { MessageState::close_dialog(); }
    else if (m_close && m_transition.in_place()) { MessageState::deactivate_state(); }
}

void MessageState::render()
{
    static constexpr int y = 229;
    const bool hasFocus    = BaseState::has_focus();

    sdl::render_rect_fill(sdl::Texture::Null, 0, 0, graphics::SCREEN_WIDTH, graphics::SCREEN_HEIGHT, colors::DIM_BACKGROUND);
    sm_dialog->render(sdl::Texture::Null, hasFocus);
    if (!m_transition.in_place()) { return; }

    sdl::text::render(sdl::Texture::Null, 312, y + 24, 20, 656, colors::WHITE, m_message);
    sdl::render_line(sdl::Texture::Null, 280, y + 192, 999, y + 192, colors::DIV_COLOR);
    sdl::text::render(sdl::Texture::Null, sm_okX, y + 214, 22, sdl::text::NO_WRAP, colors::WHITE, sm_okText);
}

//                      ---- Private functions ----

void MessageState::initialize_static_members()
{
    static constexpr int HALF_WIDTH             = 640;
    static constexpr std::string_view POP_SOUND = "ConfirmPop";
    static constexpr const char *POP_PATH       = "romfs:/Sound/ConfirmPop.wav";

    if (sm_okText && sm_dialog && sm_dialogPop) { return; }

    sm_okText    = strings::get_by_name(strings::names::YES_NO_OK, 2);
    sm_okX       = HALF_WIDTH - (sdl::text::get_width(22, sm_okText) / 2);
    sm_dialog    = ui::DialogBox::create(0, 0, 0, 0);
    sm_dialogPop = sdl::SoundManager::load(POP_SOUND, POP_PATH);
    sm_dialog->set_from_transition(m_transition, true);
}

void MessageState::close_dialog()
{
    m_close = true;
    m_transition.set_target_width(32);
    m_transition.set_target_height(32);
}

void MessageState::deactivate_state()
{
    FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_END, colors::ALPHA_FADE_BEGIN, nullptr);
    BaseState::deactivate();
}

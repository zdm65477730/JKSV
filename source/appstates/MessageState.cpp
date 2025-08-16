#include "appstates/MessageState.hpp"

#include "StateManager.hpp"
#include "appstates/FadeState.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "strings/strings.hpp"

MessageState::MessageState(std::string_view message)
    : m_message(message)
{
    MessageState::initialize_static_members();
}

MessageState::~MessageState()
{
    FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_END, colors::ALPHA_FADE_BEGIN, nullptr);
}

void MessageState::update()
{
    // To do: I only use this in one place right now. I'm not sure this guards correctly here?
    const bool aPressed = input::button_pressed(HidNpadButton_A);
    m_triggerGuard      = m_triggerGuard || (aPressed && !m_triggerGuard);
    const bool finished = m_triggerGuard && aPressed;

    if (finished) { BaseState::deactivate(); }
}

void MessageState::render()
{
    const bool hasFocus = BaseState::has_focus();

    sdl::render_rect_fill(sdl::Texture::Null, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
    sm_dialog->render(sdl::Texture::Null, hasFocus);
    sdl::text::render(sdl::Texture::Null, 312, 288, 20, 656, colors::WHITE, m_message);
    sdl::render_line(sdl::Texture::Null, 280, 454, 999, 454, colors::DIV_COLOR);
    sdl::text::render(sdl::Texture::Null, sm_okX, 476, 22, sdl::text::NO_WRAP, colors::WHITE, sm_okText);
}

void MessageState::initialize_static_members()
{
    static constexpr int HALF_WIDTH = 640;
    if (sm_okText && sm_dialog) { return; }

    sm_okText = strings::get_by_name(strings::names::YES_NO_OK, 2);
    sm_okX    = HALF_WIDTH - (sdl::text::get_width(22, sm_okText) / 2);
    sm_dialog = ui::DialogBox::create(280, 262, 720, 256);
}

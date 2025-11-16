#include "appstates/AppletModeState.hpp"

#include "graphics/colors.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"

//                      ---- Construction ----

AppletModeState::AppletModeState() { AppletModeState::get_applet_mode_string(); }

//                      ---- Public functions / overrides ----

void AppletModeState::update()
{
    // This serves no purpose other that to allow the user to exit.
}

void AppletModeState::render()
{
    // Coordinates for rendering.
    static constexpr int RENDER_X   = 48;
    static constexpr int RENDER_Y   = 112;
    static constexpr int FONT_SIZE  = 24;
    static constexpr int WRAP_WIDTH = 1154;

    sdl::text::render(sdl::Texture::Null, RENDER_X, RENDER_Y, FONT_SIZE, WRAP_WIDTH, colors::WHITE, m_appletModeString);
}

//                      ---- Private functions ----

void AppletModeState::get_applet_mode_string() noexcept
{
    m_appletModeString = strings::get_by_name(strings::names::APPLET_MODE, 0);
}

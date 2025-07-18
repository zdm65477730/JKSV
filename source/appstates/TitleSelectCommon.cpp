#include "appstates/TitleSelectCommon.hpp"
#include "colors.hpp"
#include "sdl.hpp"
#include "strings.hpp"

TitleSelectCommon::TitleSelectCommon()
{
    if (m_titleControlsX == 0)
    {
        m_titleControlsX = 1220 - sdl::text::get_width(22, strings::get_by_name(strings::names::CONTROL_GUIDES, 1));
    }
}

void TitleSelectCommon::render_control_guide()
{
    const bool hasFocus = BaseState::has_focus();

    if (hasFocus)
    {
        sdl::text::render(NULL,
                          m_titleControlsX,
                          673,
                          22,
                          sdl::text::NO_TEXT_WRAP,
                          colors::WHITE,
                          strings::get_by_name(strings::names::CONTROL_GUIDES, 1));
    }
}

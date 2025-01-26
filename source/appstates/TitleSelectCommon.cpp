#include "appstates/TitleSelectCommon.hpp"
#include "colors.hpp"
#include "sdl.hpp"
#include "strings.hpp"

TitleSelectCommon::TitleSelectCommon(void)
{
    if (m_titleControlsX == 0)
    {
        m_titleControlsX = 1220 - sdl::text::getWidth(22, strings::getByName(strings::names::CONTROL_GUIDES, 1));
    }
}

void TitleSelectCommon::renderControlGuide(void)
{
    if (AppState::hasFocus())
    {
        sdl::text::render(NULL,
                          m_titleControlsX,
                          673,
                          22,
                          sdl::text::NO_TEXT_WRAP,
                          colors::WHITE,
                          strings::getByName(strings::names::CONTROL_GUIDES, 1));
    }
}

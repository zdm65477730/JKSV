#include "appstates/TitleSelectCommon.hpp"

#include "graphics/colors.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"

TitleSelectCommon::TitleSelectCommon()
{
    if (sm_controlGuide && sm_controlGuideX != 0) { return; }

    sm_controlGuide  = strings::get_by_name(strings::names::CONTROL_GUIDES, 1);
    sm_controlGuideX = 1220 - sdl::text::get_width(22, sm_controlGuide);
}

void TitleSelectCommon::render_control_guide()
{
    const bool hasFocus = BaseState::has_focus();

    if (hasFocus)
    {
        sdl::text::render(sdl::Texture::Null, sm_controlGuideX, 673, 22, sdl::text::NO_WRAP, colors::WHITE, sm_controlGuide);
    }
}

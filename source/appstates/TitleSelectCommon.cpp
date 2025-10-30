#include "appstates/TitleSelectCommon.hpp"

#include "graphics/colors.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"

//                      ---- Construction ----

TitleSelectCommon::TitleSelectCommon() { TitleSelectCommon::initialize_control_guide(); }

//                      ---- Public functions ----

void TitleSelectCommon::sub_update() { sm_controlGuide->sub_update(); }

//                      ---- Private functions ----

void TitleSelectCommon::initialize_control_guide()
{
    if (sm_controlGuide) { return; }

    sm_controlGuide = ui::ControlGuide::create(strings::get_by_name(strings::names::CONTROL_GUIDES, 1));
}

#include "input.hpp"

namespace
{
    PadState s_gamepad;
}

void input::initialize()
{
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&s_gamepad);
}

void input::update() { padUpdate(&s_gamepad); }

bool input::button_pressed(HidNpadButton button)
{
    return (s_gamepad.buttons_cur & button) && !(s_gamepad.buttons_old & button);
}

bool input::button_held(HidNpadButton button) { return (s_gamepad.buttons_cur & button) && (s_gamepad.buttons_old & button); }

bool input::button_released(HidNpadButton button)
{
    return (s_gamepad.buttons_old & button) && !(s_gamepad.buttons_cur & button);
}

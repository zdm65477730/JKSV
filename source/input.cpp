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

void input::update() noexcept { padUpdate(&s_gamepad); }

bool input::button_pressed(HidNpadButton button) noexcept
{
    return (s_gamepad.buttons_cur & button) && !(s_gamepad.buttons_old & button);
}

bool input::button_held(HidNpadButton button) noexcept
{
    return (s_gamepad.buttons_cur & button) && (s_gamepad.buttons_old & button);
}

bool input::button_released(HidNpadButton button) noexcept
{
    return (s_gamepad.buttons_old & button) && !(s_gamepad.buttons_cur & button);
}

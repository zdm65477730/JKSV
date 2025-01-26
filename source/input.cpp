#include "input.hpp"

namespace
{
    PadState s_gamepad;
}

void input::initialize(void)
{
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&s_gamepad);
}

void input::update(void)
{
    padUpdate(&s_gamepad);
}

bool input::buttonPressed(HidNpadButton button)
{
    return (s_gamepad.buttons_cur & button) && !(s_gamepad.buttons_old & button);
}

bool input::buttonHeld(HidNpadButton button)
{
    return (s_gamepad.buttons_cur & button) && (s_gamepad.buttons_old & button);
}

bool input::buttonReleased(HidNpadButton button)
{
    return (s_gamepad.buttons_old & button) && !(s_gamepad.buttons_cur & button);
}

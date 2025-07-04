#pragma once
#include <switch.h>

namespace input
{
    /// @brief Initializes PadState and input.
    void initialize();

    /// @brief Updates the PadState.
    void update();

    /// @brief Returns if a button was pressed the current frame, but not the previous.
    /// @param button Button to check.
    /// @return True if button is pressed. False if it wasn't.
    bool button_pressed(HidNpadButton button);

    /// @brief Returns if the button was pressed or held the previous and current frame.
    /// @param button Button to check.
    /// @return True if button is held. False if it isn't.
    bool button_held(HidNpadButton button);

    /// @brief Returns if the button was pressed or held the previous frame, but not the current.
    /// @param button Button to check.
    /// @return True if the button was released. False if it wasn't.
    bool button_released(HidNpadButton button);
} // namespace input

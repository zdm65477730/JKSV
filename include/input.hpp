#pragma once
#include <switch.h>

namespace input
{
    /// @brief Initializes PadState and input.
    void initialize(void);

    /// @brief Updates the PadState.
    void update(void);

    /// @brief Returns if a button was pressed the current frame, but not the previous.
    /// @param button Button to check.
    /// @return True if button is pressed. False if it wasn't.
    bool buttonPressed(HidNpadButton button);

    /// @brief Returns if the button was pressed or held the previous and current frame.
    /// @param button Button to check.
    /// @return True if button is held. False if it isn't.
    bool buttonHeld(HidNpadButton button);

    /// @brief Returns if the button was pressed or held the previous frame, but not the current.
    /// @param button Button to check.
    /// @return True if the button was released. False if it wasn't.
    bool buttonReleased(HidNpadButton button);
} // namespace input

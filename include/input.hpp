#pragma once
#include <switch.h>

namespace input
{
    /// @brief Initializes PadState and input.
    void initialize();

    /// @brief Updates the PadState.
    void update() noexcept;

    /// @brief Returns if a button was pressed the current frame, but not the previous.
    /// @param button Button to check.
    bool button_pressed(HidNpadButton button) noexcept;

    /// @brief Returns if the button was pressed or held the previous and current frame.
    /// @param button Button to check.
    bool button_held(HidNpadButton button) noexcept;

    /// @brief Returns if the button was pressed or held the previous frame, but not the current.
    /// @param button Button to check.
    bool button_released(HidNpadButton button) noexcept;
} // namespace input

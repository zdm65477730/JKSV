#pragma once
#include "sdl.hpp"
#include "ui/ColorMod.hpp"

// These are just functions to render generic parts of the UI.
namespace ui
{
    /// @brief Renders a dialog box
    /// @param target Target to render to.
    /// @param x X coordinate to render to.
    /// @param y Y coordinate to render to.
    /// @param width Width of dialog box in pixels.
    /// @param height Height of dialog box in pixels.
    void render_dialog_box(SDL_Texture *target, int x, int y, int width, int height);

    /// @brief Renders a bounding box.
    /// @param target Target to render to.
    /// @param x X coordinate to render to.
    /// @param y Y coordinate to render to.
    /// @param width Width of dialog box in pixels.
    /// @param height Height of dialog box in pixels.
    /// @param colorMod Color to multiply in rendering.
    void render_bounding_box(SDL_Texture *target, int x, int y, int width, int height, const ui::ColorMod &colorMod);
} // namespace ui

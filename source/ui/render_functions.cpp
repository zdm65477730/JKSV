#include "ui/render_functions.hpp"

#include "colors.hpp"

namespace
{
    sdl::SharedTexture s_dialogCorners       = nullptr;
    sdl::SharedTexture s_menuBoundingCorners = nullptr;
} // namespace

void ui::render_dialog_box(SDL_Texture *target, int x, int y, int width, int height)
{
    if (!s_dialogCorners)
    {
        s_dialogCorners = sdl::TextureManager::create_load_texture("DialogCorners", "romfs:/Textures/DialogCorners.png");
    }

    // Top
    s_dialogCorners->render_part(target, x, y, 0, 0, 16, 16);
    sdl::render_rect_fill(target, x + 16, y, width - 32, 16, colors::DIALOG_BOX);
    s_dialogCorners->render_part(target, (x + width) - 16, y, 16, 0, 16, 16);
    // Middle
    sdl::render_rect_fill(NULL, x, y + 16, width, height - 32, colors::DIALOG_BOX);
    // Bottom
    s_dialogCorners->render_part(target, x, (y + height) - 16, 0, 16, 16, 16);
    sdl::render_rect_fill(NULL, x + 16, (y + height) - 16, width - 32, 16, colors::DIALOG_BOX);
    s_dialogCorners->render_part(NULL, (x + width) - 16, (y + height) - 16, 16, 16, 16, 16);
}

void ui::render_bounding_box(SDL_Texture *target, int x, int y, int width, int height, const ui::ColorMod &colorMod)
{
    if (!s_menuBoundingCorners)
    {
        s_menuBoundingCorners =
            sdl::TextureManager::create_load_texture("MenuBoundingCorners", "romfs:/Textures/MenuBounding.png");
    }

    // This shouldn't fail, but I don't really care if it does.
    s_menuBoundingCorners->set_color_mod(colorMod);

    // Top
    s_menuBoundingCorners->render_part(target, x, y, 0, 0, 8, 8);
    sdl::render_rect_fill(target, x + 8, y, width - 16, 4, colorMod);
    s_menuBoundingCorners->render_part(target, (x + width) - 8, y, 8, 0, 8, 8);
    // Middle
    sdl::render_rect_fill(target, x, y + 8, 4, height - 16, colorMod);
    sdl::render_rect_fill(target, (x + width) - 4, y + 8, 4, height - 16, colorMod);
    // Bottom
    s_menuBoundingCorners->render_part(target, x, (y + height) - 8, 0, 8, 8, 8);
    sdl::render_rect_fill(target, x + 8, (y + height) - 4, width - 16, 4, colorMod);
    s_menuBoundingCorners->render_part(target, (x + width) - 8, (y + height) - 8, 8, 8, 8, 8);
}

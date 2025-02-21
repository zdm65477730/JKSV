#include "ui/renderFunctions.hpp"
#include "colors.hpp"

namespace
{
    sdl::SharedTexture s_dialogCorners = nullptr;
    sdl::SharedTexture s_menuBoundingCorners = nullptr;
} // namespace

void ui::render_dialog_box(SDL_Texture *target, int x, int y, int width, int height)
{
    if (!s_dialogCorners)
    {
        s_dialogCorners = sdl::TextureManager::createLoadTexture("DialogCorners", "romfs:/Textures/DialogCorners.png");
    }

    // Top
    s_dialogCorners->renderPart(target, x, y, 0, 0, 16, 16);
    sdl::renderRectFill(target, x + 16, y, width - 32, 16, colors::DIALOG_BOX);
    s_dialogCorners->renderPart(target, (x + width) - 16, y, 16, 0, 16, 16);
    // Middle
    sdl::renderRectFill(NULL, x, y + 16, width, height - 32, colors::DIALOG_BOX);
    // Bottom
    s_dialogCorners->renderPart(target, x, (y + height) - 16, 0, 16, 16, 16);
    sdl::renderRectFill(NULL, x + 16, (y + height) - 16, width - 32, 16, colors::DIALOG_BOX);
    s_dialogCorners->renderPart(NULL, (x + width) - 16, (y + height) - 16, 16, 16, 16, 16);
}

void ui::render_bounding_box(SDL_Texture *target, int x, int y, int width, int height, uint8_t colorMod)
{
    if (!s_menuBoundingCorners)
    {
        s_menuBoundingCorners =
            sdl::TextureManager::createLoadTexture("MenuBoundingCorners", "romfs:/Textures/MenuBounding.png");
    }

    // Setup color.
    sdl::Color renderMod = {static_cast<uint32_t>((0x88 + colorMod) << 16 | (0xC5 + (colorMod / 2)) << 8 | 0xFF)};

    // This shouldn't fail, but I don't really care if it does.
    s_menuBoundingCorners->setColorMod(renderMod);

    // Top
    s_menuBoundingCorners->renderPart(target, x, y, 0, 0, 8, 8);
    sdl::renderRectFill(target, x + 8, y, width - 16, 4, renderMod);
    s_menuBoundingCorners->renderPart(target, (x + width) - 8, y, 8, 0, 8, 8);
    // Middle
    sdl::renderRectFill(target, x, y + 8, 4, height - 16, renderMod);
    sdl::renderRectFill(target, (x + width) - 4, y + 8, 4, height - 16, renderMod);
    // Bottom
    s_menuBoundingCorners->renderPart(target, x, (y + height) - 8, 0, 8, 8, 8);
    sdl::renderRectFill(target, x + 8, (y + height) - 4, width - 16, 4, renderMod);
    s_menuBoundingCorners->renderPart(target, (x + width) - 8, (y + height) - 8, 8, 8, 8, 8);
}

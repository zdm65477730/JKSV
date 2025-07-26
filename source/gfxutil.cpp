#include "gfxutil.hpp"

namespace
{
    /// @brief Width of generic icons in pixels.
    constexpr int SIZE_ICON_WIDTH = 256;

    /// @brief Height of generic icons in pixels.
    constexpr int SIZE_ICON_HEIGHT = 256;
} // namespace

sdl::SharedTexture gfxutil::create_generic_icon(std::string_view text,
                                                int fontSize,
                                                sdl::Color background,
                                                sdl::Color foreground)
{
    // Create base icon texture.
    sdl::SharedTexture icon =
        sdl::TextureManager::create_load_texture(text, 256, 256, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);

    // Get the centered X and Y coordinates.
    const int textX = (SIZE_ICON_WIDTH / 2) - (sdl::text::get_width(fontSize, text.data()) / 2);
    const int textY = (SIZE_ICON_HEIGHT / 2) - (fontSize / 2);

    icon->clear(background);
    sdl::text::render(icon->get(), textX, textY, fontSize, sdl::text::NO_TEXT_WRAP, foreground, text.data());

    return icon;
}

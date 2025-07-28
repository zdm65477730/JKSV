#include "ui/TitleTile.hpp"

#include "colors.hpp"
#include "logger.hpp"

ui::TitleTile::TitleTile(bool isFavorite, sdl::SharedTexture icon)
    : m_isFavorite(isFavorite)
    , m_icon(icon) {};

void ui::TitleTile::update(bool isSelected)
{
    static constexpr int BASE_WIDTH   = 128;
    static constexpr int EXPAND_WIDTH = 176;
    static constexpr int INCREASE     = 16;
    static constexpr int DECREASE     = 8;

    if (isSelected && m_renderWidth != EXPAND_WIDTH)
    {
        // I think it's safe to assume both are too small.
        m_renderWidth += INCREASE;
        m_renderHeight += INCREASE;
    }
    else if (!isSelected && m_renderWidth != BASE_WIDTH)
    {
        m_renderWidth -= DECREASE;
        m_renderHeight -= DECREASE;
    }
}

void ui::TitleTile::render(SDL_Texture *target, int x, int y)
{
    const int renderX = x - ((m_renderWidth - 128) / 2);
    const int renderY = y - ((m_renderHeight - 128) / 2);

    m_icon->render_stretched(target, renderX, renderY, m_renderWidth, m_renderHeight);
    if (m_isFavorite)
    {
        sdl::text::render(target, renderX + 2, renderY + 2, 28, sdl::text::NO_TEXT_WRAP, colors::PINK, "\uE017");
    }
}

void ui::TitleTile::reset()
{
    m_renderWidth  = 128;
    m_renderHeight = 128;
}

int ui::TitleTile::get_width() const { return m_renderWidth; }

int ui::TitleTile::get_height() const { return m_renderHeight; }

#include "ui/TitleTile.hpp"
#include "colors.hpp"

ui::TitleTile::TitleTile(bool isFavorite, sdl::SharedTexture icon) : m_isFavorite(isFavorite), m_icon(icon) {};

void ui::TitleTile::update(bool isSelected)
{
    if (isSelected && m_renderWidth < 164)
    {
        // I think it's safe to assume both are too small.
        m_renderWidth += 18;
        m_renderHeight += 18;
    }
    else if (!isSelected && m_renderWidth > 128)
    {
        m_renderWidth -= 9;
        m_renderHeight -= 9;
    }
}

void ui::TitleTile::render(SDL_Texture *target, int x, int y)
{
    int renderX = x - ((m_renderWidth - 128) / 2);
    int renderY = y - ((m_renderHeight - 128) / 2);

    m_icon->renderStretched(target, renderX, renderY, m_renderWidth, m_renderHeight);
    if (m_isFavorite)
    {
        sdl::text::render(target, renderX + 4, renderY + 2, 28, sdl::text::NO_TEXT_WRAP, colors::PINK, "\uE017");
    }
}

void ui::TitleTile::reset(void)
{
    m_renderWidth = 128;
    m_renderHeight = 128;
}

int ui::TitleTile::get_width(void) const
{
    return m_renderWidth;
}

int ui::TitleTile::get_height(void) const
{
    return m_renderHeight;
}

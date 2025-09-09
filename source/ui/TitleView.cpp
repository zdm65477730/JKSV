#include "ui/TitleView.hpp"

#include "config/config.hpp"
#include "error.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "logging/logger.hpp"

#include <cmath>

namespace
{
    constexpr int ICON_ROW_SIZE = 7;
}

ui::TitleView::TitleView(data::User *user)
    : m_user(user)
    , m_bounding(ui::BoundingBox::create(0, 0, 188, 188))
{
    TitleView::refresh();
}

void ui::TitleView::update(bool hasFocus)
{
    if (m_titleTiles.empty() || !hasFocus) { return; }

    m_bounding->update(hasFocus);
    TitleView::handle_input();
    TitleView::handle_scrolling();
    TitleView::update_tiles();
}

void ui::TitleView::render(sdl::SharedTexture &target, bool hasFocus)
{
    static constexpr int TILE_SPACE_VERT = 144;
    static constexpr int TILE_SPACE_HOR  = 144;

    if (m_titleTiles.empty()) { return; }

    const int tileCount = m_titleTiles.size();
    for (int i = 0, tempY = m_y; i < tileCount; i += ICON_ROW_SIZE, tempY += TILE_SPACE_VERT)
    {
        const int endRow = i + ICON_ROW_SIZE;
        for (int j = i, tempX = 32; j < endRow && j < tileCount; j++, tempX += TILE_SPACE_HOR)
        {
            if (j == m_selected)
            {
                m_selectedX = tempX;
                m_selectedY = tempY;
                continue;
            }

            ui::TitleTile &tile = m_titleTiles[j];
            tile.render(target, tempX, tempY);
        }
    }

    if (hasFocus)
    {
        m_bounding->set_x(m_selectedX - 30);
        m_bounding->set_y(m_selectedY - 30);
        sdl::render_rect_fill(target, m_selectedX - 28, m_selectedY - 28, 184, 184, colors::CLEAR_COLOR);
        m_bounding->render(target, hasFocus);
    }

    ui::TitleTile &selectedTile = m_titleTiles[m_selected];
    selectedTile.render(target, m_selectedX, m_selectedY);
}

int ui::TitleView::get_selected() const noexcept { return m_selected; }

void ui::TitleView::set_selected(int selected) noexcept
{
    const int tilesCount = m_titleTiles.size();
    if (selected < 0 || selected >= tilesCount) { return; }
    m_selected = selected;
}

void ui::TitleView::refresh()
{
    m_titleTiles.clear();

    const int entryCount = m_user->get_total_data_entries();

    for (int i = 0; i < entryCount; i++)
    {
        const uint64_t applicationID = m_user->get_application_id_at(i);
        data::TitleInfo *titleInfo   = data::get_title_info_by_id(applicationID);
        if (error::is_null(titleInfo)) { continue; }

        const bool isFavorite   = config::is_favorite(applicationID);
        sdl::SharedTexture icon = titleInfo->get_icon(); // I don't like this but w/e.

        m_titleTiles.emplace_back(isFavorite, i, icon);
    }

    const int tileCount = m_titleTiles.size() - 1;
    if (tileCount <= 0) { m_selected = 0; }
    else if (m_selected > tileCount) { m_selected = tileCount; }
}

void ui::TitleView::reset()
{
    for (ui::TitleTile &currentTile : m_titleTiles) { currentTile.reset(); }
}

void ui::TitleView::handle_input()
{
    const int totalTiles        = m_titleTiles.size() - 1;
    const bool upPressed        = input::button_pressed(HidNpadButton_AnyUp);
    const bool downPressed      = input::button_pressed(HidNpadButton_AnyDown);
    const bool leftPressed      = input::button_pressed(HidNpadButton_AnyLeft);
    const bool rightPressed     = input::button_pressed(HidNpadButton_AnyRight);
    const bool lShoulderPressed = input::button_pressed(HidNpadButton_L);
    const bool rShoulderPressed = input::button_pressed(HidNpadButton_R);

    if (upPressed) { m_selected -= ICON_ROW_SIZE; }
    else if (leftPressed) { --m_selected; }
    else if (lShoulderPressed) { m_selected -= ICON_ROW_SIZE * 3; }
    else if (downPressed) { m_selected += ICON_ROW_SIZE; }
    else if (rightPressed) { ++m_selected; }
    else if (rShoulderPressed) { m_selected += ICON_ROW_SIZE * 3; }

    if (m_selected < 0) { m_selected = 0; }
    if (m_selected > totalTiles) { m_selected = totalTiles; }
}

void ui::TitleView::handle_scrolling()
{
    static constexpr double UPPER_THRESHOLD = 32.0f;
    static constexpr double LOWER_THRESHOLD = 388.0f;

    const double scaling = config::get_animation_scaling();

    if (m_selectedY < UPPER_THRESHOLD)
    {
        const double shiftDown = (UPPER_THRESHOLD - m_selectedY) / scaling;
        m_y += std::round(shiftDown);
    }
    else if (m_selectedY > LOWER_THRESHOLD)
    {
        const double shiftUp = (LOWER_THRESHOLD - m_selectedY) / scaling;
        m_y += std::round(shiftUp);
    }
}

void ui::TitleView::update_tiles()
{
    for (ui::TitleTile &tile : m_titleTiles) { tile.update(m_selected); }
}

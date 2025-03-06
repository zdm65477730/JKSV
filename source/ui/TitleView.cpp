#include "ui/TitleView.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "ui/render_functions.hpp"
#include <cmath>

namespace
{
    constexpr int ICON_ROW_SIZE = 7;
}

ui::TitleView::TitleView(data::User *user) : m_user(user)
{
    TitleView::refresh();
}

void ui::TitleView::update(bool hasFocus)
{
    if (m_titleTiles.empty())
    {
        return;
    }

    // Update pulse
    if (hasFocus)
    {
        m_colorMod.update();
    }

    // Input.
    int totalTiles = m_titleTiles.size() - 1;
    if (input::button_pressed(HidNpadButton_AnyUp) && (m_selected -= ICON_ROW_SIZE) < 0)
    {
        m_selected = 0;
    }
    else if (input::button_pressed(HidNpadButton_AnyDown) && (m_selected += ICON_ROW_SIZE) > totalTiles)
    {
        m_selected = totalTiles;
    }
    else if (input::button_pressed(HidNpadButton_AnyLeft) && m_selected > 0)
    {
        --m_selected;
    }
    else if (input::button_pressed(HidNpadButton_AnyRight) && m_selected < totalTiles)
    {
        ++m_selected;
    }
    else if (input::button_pressed(HidNpadButton_L) && (m_selected -= 21) < 0)
    {
        m_selected = 0;
    }
    else if (input::button_pressed(HidNpadButton_R) && (m_selected += 21) > totalTiles)
    {
        m_selected = totalTiles;
    }

    double scaling = config::get_animation_scaling();
    if (m_selectedY > 388.0f)
    {
        m_y += std::ceil((388.0f - m_selectedY) / scaling);
    }
    else if (m_selectedY < 28.0f)
    {
        m_y += std::ceil((28.0f - m_selectedY) / scaling);
    }

    for (size_t i = 0; i < m_titleTiles.size(); i++)
    {
        m_titleTiles.at(i).update(m_selected == static_cast<int>(i) ? true : false);
    }
}

void ui::TitleView::render(SDL_Texture *target, bool hasFocus)
{
    if (m_titleTiles.empty())
    {
        return;
    }

    for (int i = 0, tempY = m_y; i < static_cast<int>(m_titleTiles.size()); tempY += 144)
    {
        int endRow = i + 7;
        for (int j = i, tempX = 32; j < endRow; j++, i++, tempX += 144)
        {
            if (i >= static_cast<int>(m_titleTiles.size()))
            {
                break;
            }

            // Save the X and Y to render the selected tile over the rest.
            if (i == m_selected)
            {
                m_selectedX = tempX;
                m_selectedY = tempY;
                continue;
            }
            // Just render
            m_titleTiles.at(i).render(target, tempX, tempY);
        }
    }
    // Now render the selected title.
    if (hasFocus)
    {
        sdl::render_rect_fill(target, m_selectedX - 23, m_selectedY - 23, 174, 174, colors::CLEAR_COLOR);
        ui::render_bounding_box(target, m_selectedX - 24, m_selectedY - 24, 176, 176, m_colorMod);
    }
    m_titleTiles.at(m_selected).render(target, m_selectedX, m_selectedY);
}

int ui::TitleView::get_selected(void) const
{
    return m_selected;
}

void ui::TitleView::refresh(void)
{
    m_titleTiles.clear();
    for (size_t i = 0; i < m_user->get_total_data_entries(); i++)
    {
        // Get pointer to data from user save index I.
        data::TitleInfo *currentTitleInfo = data::get_title_info_by_id(m_user->get_application_id_at(i));
        // Emplace is faster than push
        m_titleTiles.emplace_back(config::is_favorite(m_user->get_application_id_at(i)), currentTitleInfo->get_icon());
    }
}

void ui::TitleView::reset(void)
{
    for (ui::TitleTile &currentTile : m_titleTiles)
    {
        currentTile.reset();
    }
}

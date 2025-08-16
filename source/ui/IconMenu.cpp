#include "ui/IconMenu.hpp"

#include "graphics/colors.hpp"

namespace
{
    constexpr int ICON_RENDER_WIDTH  = 128;
    constexpr int ICON_RENDER_HEIGHT = 128;

    constexpr int BOUND_WIDTH  = 152;
    constexpr int BOUND_HEIGHT = 146;
}

ui::IconMenu::IconMenu(int x, int y, int renderTargetHeight)
    : Menu(x, y, 152, 80, renderTargetHeight)
{
    // This needs to be overriden from the base state.
    m_boundingBox->set_width_height(152, 146);
}

void ui::IconMenu::update(bool hasFocus) { Menu::update(hasFocus); }

void ui::IconMenu::render(sdl::SharedTexture &target, bool hasFocus)
{
    const int optionCount = m_options.size();
    for (int i = 0, tempY = m_y; i < optionCount; i++, tempY += m_optionHeight)
    {
        // Clear target.
        m_optionTarget->clear(colors::TRANSPARENT);
        if (i == m_selected)
        {
            if (hasFocus)
            {
                m_boundingBox->set_xy(m_x - 8, tempY - 8);
                m_boundingBox->render(target, hasFocus);
            }
            // This is always rendered.
            sdl::render_rect_fill(m_optionTarget, 0, 0, 4, 130, colors::BLUE_GREEN);
        }
        m_options[i]->render_stretched(m_optionTarget, 8, 1, ICON_RENDER_WIDTH, ICON_RENDER_HEIGHT);
        m_optionTarget->render(target, m_x, tempY);
    }
}

void ui::IconMenu::add_option(sdl::SharedTexture newOption)
{
    Menu::add_option("ICON"); // Parent class needs text for this to work correctly.
    m_options.push_back(newOption);
}

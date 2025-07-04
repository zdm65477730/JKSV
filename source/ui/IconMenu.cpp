#include "ui/IconMenu.hpp"
#include "colors.hpp"
#include "ui/render_functions.hpp"

ui::IconMenu::IconMenu(int x, int y, int rendertargetHeight) : Menu(x, y, 152, 80, rendertargetHeight) {};

void ui::IconMenu::update(bool hasFocus)
{
    Menu::update(hasFocus);
}

void ui::IconMenu::render(SDL_Texture *target, bool hasFocus)
{
    if (hasFocus)
    {
        m_colorMod.update();
    }

    for (int i = 0, tempY = m_y; i < static_cast<int>(m_options.size()); i++, tempY += m_optionHeight)
    {
        // Clear target.
        m_optionTarget->clear(colors::TRANSPARENT);
        if (i == m_selected)
        {
            if (hasFocus)
            {
                ui::render_bounding_box(target, m_x - 8, tempY - 8, 152, 146, m_colorMod);
            }
            sdl::render_rect_fill(m_optionTarget->get(), 0, 0, 4, 130, {0x00FFC5FF});
        }
        m_options.at(i)->render_stretched(m_optionTarget->get(), 8, 1, 128, 128);
        m_optionTarget->render(target, m_x, tempY);
    }
}

void ui::IconMenu::add_option(sdl::SharedTexture newOption)
{
    // Parent needs a text option to work correctly.
    Menu::add_option("ICON");
    m_options.push_back(newOption);
}

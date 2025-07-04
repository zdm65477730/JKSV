#include "ui/Menu.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "input.hpp"
#include "ui/render_functions.hpp"
#include <cmath>

ui::Menu::Menu(int x, int y, int width, int fontSize, int renderTargetHeight)
    : m_x(x), m_y(y), m_optionHeight(std::ceil(static_cast<double>(fontSize) * 1.8f)), m_originalY(y), m_targetY(y),
      m_width(width), m_fontSize(fontSize), m_renderTargetHeight(renderTargetHeight)
{
    // Create render target for options
    static int MENU_ID = 0;
    std::string menuTargetName = "MENU_" + std::to_string(MENU_ID++);
    m_optionTarget = sdl::TextureManager::create_load_texture(menuTargetName,
                                                              m_width,
                                                              m_optionHeight,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);

    // Calculate around how many options can be shown on the render target at once.
    m_maxDisplayOptions = (renderTargetHeight - m_originalY) / m_optionHeight;
    m_scrollLength = std::floor(static_cast<double>(m_maxDisplayOptions) / 2.0f);
}

void ui::Menu::update(bool hasFocus)
{
    // Bail if there's nothing to update.
    if (m_options.empty())
    {
        return;
    }

    int optionsSize = m_options.size();
    if (input::button_pressed(HidNpadButton_AnyUp) && --m_selected < 0)
    {
        m_selected = optionsSize - 1;
    }
    else if (input::button_pressed(HidNpadButton_AnyDown) && ++m_selected >= optionsSize)
    {
        m_selected = 0;
    }
    else if (input::button_pressed(HidNpadButton_AnyLeft) && (m_selected -= m_scrollLength) < 0)
    {
        m_selected = 0;
    }
    else if (input::button_pressed(HidNpadButton_AnyRight) && (m_selected += m_scrollLength) >= optionsSize)
    {
        m_selected = optionsSize - 1;
    }
    else if (input::button_pressed(HidNpadButton_L) && (m_selected -= m_scrollLength * 3) < 0)
    {
        m_selected = 0;
    }
    else if (input::button_pressed(HidNpadButton_R) && (m_selected += m_scrollLength * 3) >= optionsSize)
    {
        m_selected = optionsSize - 1;
    }

    // Don't bother continuing further if there's no reason to scroll.
    if (static_cast<int>(m_options.size()) <= m_maxDisplayOptions)
    {
        return;
    }

    if (m_selected < m_scrollLength)
    {
        m_targetY = m_originalY;
    }
    else if (m_selected >= static_cast<int>(m_options.size()) - (m_maxDisplayOptions - m_scrollLength))
    {
        m_targetY = m_originalY - ((m_options.size() - m_maxDisplayOptions) * m_optionHeight);
    }
    else if (m_selected >= m_scrollLength)
    {
        m_targetY = m_originalY - ((m_selected - m_scrollLength) * m_optionHeight);
    }

    if (m_y != m_targetY)
    {
        m_y += std::ceil((m_targetY - m_y) / config::get_animation_scaling());
    }
}

void ui::Menu::render(SDL_Texture *target, bool hasFocus)
{
    if (m_options.empty())
    {
        return;
    }

    m_colorMod.update();

    // I hate doing this.
    int targetHeight = 0;
    SDL_QueryTexture(target, NULL, NULL, NULL, &targetHeight);
    for (int i = 0, tempY = m_y; i < static_cast<int>(m_options.size()); i++, tempY += m_optionHeight)
    {
        if (tempY < -m_fontSize)
        {
            continue;
        }
        else if (tempY > m_renderTargetHeight)
        {
            // This is safe to break the loop for.
            break;
        }

        // Clear target texture.
        m_optionTarget->clear(colors::TRANSPARENT);

        if (i == m_selected)
        {
            if (hasFocus)
            {
                // render the bounding box
                ui::render_bounding_box(target, m_x - 4, tempY - 4, m_width + 8, m_optionHeight + 8, m_colorMod);
            }
            // render the little rectangle.
            sdl::render_rect_fill(m_optionTarget->get(), 8, 8, 4, m_optionHeight - 16, colors::BLUE_GREEN);
        }
        // render text to target.
        sdl::text::render(m_optionTarget->get(),
                          24,
                          ((m_optionHeight / 2) - (m_fontSize / 2)) - 1,
                          m_fontSize,
                          sdl::text::NO_TEXT_WRAP,
                          i == m_selected && hasFocus ? colors::BLUE_GREEN : colors::WHITE,
                          m_options.at(i).c_str());
        // render target to target
        m_optionTarget->render(target, m_x, tempY);
    }
}

void ui::Menu::add_option(std::string_view newOption)
{
    m_options.push_back(newOption.data());
}

void ui::Menu::edit_option(int index, std::string_view newOption)
{
    if (index < 0 || index >= static_cast<int>(m_options.size()))
    {
        return;
    }
    m_options[index] = newOption.data();
}

int ui::Menu::get_selected() const
{
    return m_selected;
}

void ui::Menu::set_selected(int selected)
{
    m_selected = selected;
}

void ui::Menu::set_width(int width)
{
    m_width = width;
}

void ui::Menu::reset()
{
    m_selected = 0;
    m_y = m_originalY;
    m_options.clear();
}

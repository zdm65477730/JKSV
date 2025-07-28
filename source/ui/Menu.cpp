#include "ui/Menu.hpp"

#include "colors.hpp"
#include "config.hpp"
#include "input.hpp"
#include "ui/render_functions.hpp"

#include <cmath>

ui::Menu::Menu(int x, int y, int width, int fontSize, int renderTargetHeight)
    : m_x(x)
    , m_y(y)
    , m_optionHeight(std::ceil(static_cast<double>(fontSize) * 1.8f))
    , m_originalY(y)
    , m_targetY(y)
    , m_width(width)
    , m_fontSize(fontSize)
    , m_textY((m_optionHeight / 2) - (m_fontSize / 2) - 1) // This seems to be the best alignment.
    , m_renderTargetHeight(renderTargetHeight)
    , m_optionScroll("temp", 16, 0, m_width - 6, m_optionHeight, m_fontSize, colors::BLUE_GREEN, colors::TRANSPARENT)
{
    // Create render target for options
    static int MENU_ID         = 0;
    std::string menuTargetName = "MENU_" + std::to_string(MENU_ID++);
    m_optionTarget             = sdl::TextureManager::create_load_texture(menuTargetName,
                                                              m_width,
                                                              m_optionHeight,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);

    // Calculate around how many options can be shown on the render target at once.
    m_maxDisplayOptions = (renderTargetHeight - m_originalY) / m_optionHeight;
    m_scrollLength      = std::floor(static_cast<double>(m_maxDisplayOptions) / 2.0f);
}

void ui::Menu::update(bool hasFocus)
{
    if (m_options.empty()) { return; }

    m_optionScroll.update(hasFocus);

    const bool upPressed        = input::button_pressed(HidNpadButton_AnyUp);
    const bool downPressed      = input::button_pressed(HidNpadButton_AnyDown);
    const bool leftPressed      = input::button_pressed(HidNpadButton_AnyLeft);
    const bool rightPressed     = input::button_pressed(HidNpadButton_AnyRight);
    const bool lShoulderPressed = input::button_pressed(HidNpadButton_L);
    const bool rShoulderPressed = input::button_pressed(HidNpadButton_R);
    const int optionsSize       = m_options.size();
    const int prevSelected      = m_selected;

    if (upPressed) { --m_selected; }
    else if (downPressed) { ++m_selected; }
    else if (leftPressed) { m_selected -= m_scrollLength; }
    else if (rightPressed) { m_selected += m_scrollLength; }
    else if (lShoulderPressed) { m_selected -= m_scrollLength * 3; }
    else if (rShoulderPressed) { m_selected += m_scrollLength * 3; }

    if (m_selected < 0) { m_selected = 0; }
    else if (m_selected >= optionsSize) { m_selected = optionsSize - 1; }

    // This needs updating.
    if (m_selected != prevSelected) { m_optionScroll.set_text(m_options[m_selected], false); }

    // Don't bother continuing further if there's no reason to scroll.
    if (optionsSize <= m_maxDisplayOptions) { return; }

    // These are to make things slightly easier to read down here.
    const int endScrollPoint = optionsSize - (m_maxDisplayOptions - m_scrollLength);
    const int scrolledItems  = m_selected - m_scrollLength;
    const double scaling     = config::get_animation_scaling();

    if (m_selected < m_scrollLength) { m_targetY = m_originalY; } // Don't bother. There's no point.
    else if (m_selected >= endScrollPoint) { m_targetY = m_originalY - (optionsSize - m_maxDisplayOptions) * m_optionHeight; }
    else if (m_selected >= m_scrollLength) { m_targetY = m_originalY - (scrolledItems * m_optionHeight); }

    if (m_y != m_targetY) { m_y += std::ceil((m_targetY - m_y) / scaling); }
}

void ui::Menu::render(SDL_Texture *target, bool hasFocus)
{
    if (m_options.empty()) { return; }
    SDL_Texture *optionTarget = m_optionTarget->get();

    m_colorMod.update();

    // I hate doing this.
    const int optionSize = m_options.size();
    for (int i = 0, tempY = m_y; i < optionSize; i++, tempY += m_optionHeight)
    {
        if (tempY < -m_fontSize) { continue; }
        else if (tempY > m_renderTargetHeight) { break; }

        m_optionTarget->clear(colors::TRANSPARENT);

        if (i == m_selected)
        {
            if (hasFocus) { ui::render_bounding_box(target, m_x - 4, tempY - 4, m_width + 8, m_optionHeight + 8, m_colorMod); }
            sdl::render_rect_fill(optionTarget, 8, 8, 4, m_optionHeight - 16, colors::BLUE_GREEN);
            m_optionScroll.render(optionTarget, hasFocus);
        }
        else
        {
            const char *option = m_options[i].c_str();
            sdl::text::render(optionTarget, 24, m_textY, m_fontSize, sdl::text::NO_TEXT_WRAP, colors::WHITE, option);
        }
        // render target to target
        m_optionTarget->render(target, m_x, tempY);
    }
}

void ui::Menu::add_option(std::string_view newOption)
{
    // This is needed. The initialization is just a temporary one.
    if (m_options.empty()) { m_optionScroll.set_text(newOption, false); }
    m_options.push_back(newOption.data());
}

void ui::Menu::edit_option(int index, std::string_view newOption)
{
    const int optionSize = m_options.size();
    if (index < 0 || index >= optionSize) { return; }
    m_options[index] = newOption.data();
}

int ui::Menu::get_selected() const { return m_selected; }

void ui::Menu::set_selected(int selected) { m_selected = selected; }

void ui::Menu::set_width(int width) { m_width = width; }

void ui::Menu::reset()
{
    m_selected = 0;
    m_y        = m_originalY;
    m_options.clear();
}

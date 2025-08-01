#include "ui/SlideOutPanel.hpp"

#include "colors.hpp"
#include "config.hpp"
#include "mathutil.hpp"

#include <cmath>
#include <utility>

namespace
{
    constexpr int SCREEN_WIDTH = 1280;
}

ui::SlideOutPanel::SlideOutPanel(int width, Side side)
    : m_x{side == Side::Left ? static_cast<double>(-width) : static_cast<double>(SCREEN_WIDTH)}
    , m_width{width}
    , m_targetX{side == Side::Left ? 0.0f : static_cast<double>(SCREEN_WIDTH) - m_width}
    , m_side{side}
    , m_scaling{config::get_animation_scaling()}
{
    static constexpr int sdlFlags = SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET;

    static int targetID    = 0;
    std::string targetName = "panelTarget_" + std::to_string(targetID++);
    m_renderTarget         = sdl::TextureManager::create_load_texture(targetName, width, 720, sdlFlags);
}

void ui::SlideOutPanel::update(bool hasFocus)
{
    const bool openingFromLeft  = !m_isOpen && m_side == Side::Left && m_x < m_targetX;
    const bool openingFromRight = !m_isOpen && m_side == Side::Right && m_x > m_targetX;

    if (openingFromLeft) { SlideOutPanel::slide_out_left(); }
    else if (openingFromRight) { SlideOutPanel::slide_out_right(); }

    if (m_isOpen)
    {
        for (auto &currentElement : m_elements) { currentElement->update(hasFocus); }
    }
}

void ui::SlideOutPanel::render(SDL_Texture *Target, bool hasFocus)
{
    for (auto &currentElement : m_elements) { currentElement->render(m_renderTarget->get(), hasFocus); }

    m_renderTarget->render(NULL, m_x, 0);
}

void ui::SlideOutPanel::clear_target() { m_renderTarget->clear(colors::SLIDE_PANEL_CLEAR); }

void ui::SlideOutPanel::reset()
{
    m_x          = m_side == Side::Left ? -(m_width) : SCREEN_WIDTH;
    m_isOpen     = false;
    m_closePanel = false;
}

void ui::SlideOutPanel::close() { m_closePanel = true; }

bool ui::SlideOutPanel::is_open() const { return m_isOpen; }

bool ui::SlideOutPanel::is_closed()
{
    // I'm assuming this is going to be called, waiting for the panel to close so.
    const double scaling    = config::get_animation_scaling();
    const bool closeToLeft  = m_closePanel && m_side == Side::Left && m_x > -m_width;
    const bool closeToRight = m_closePanel && m_side == Side::Right && m_x < SCREEN_WIDTH;
    if (closeToLeft) { m_x += -(m_width - m_x) / scaling; }
    else if (closeToRight) { m_x += m_x / scaling; }

    const bool closed = m_side == Side::Left ? m_x <= -m_width : m_x >= SCREEN_WIDTH;
    return m_closePanel && closed;
}

void ui::SlideOutPanel::push_new_element(std::shared_ptr<ui::Element> newElement) { m_elements.push_back(newElement); }

void ui::SlideOutPanel::clear_elements() { m_elements.clear(); }

SDL_Texture *ui::SlideOutPanel::get_target() { return m_renderTarget->get(); }

void ui::SlideOutPanel::slide_out_left()
{
    m_x -= std::round(m_x / m_scaling);

    // This is a workaround for the floating points never lining up quite right.
    const int distance = math::Util<double>::get_absolute_distance(m_x, m_targetX);
    if (distance <= 2)
    {
        m_x      = m_targetX;
        m_isOpen = true;
    }
}

void ui::SlideOutPanel::slide_out_right()
{
    const double screenWidth = static_cast<double>(SCREEN_WIDTH);
    const double width       = static_cast<double>(m_width);
    const double pixels      = (screenWidth - width - m_x) / m_scaling;
    m_x += std::round(pixels);

    const int distance = math::Util<double>::get_absolute_distance(m_x, m_targetX);
    if (distance <= 2)
    {
        m_x      = m_targetX;
        m_isOpen = true;
    }
}

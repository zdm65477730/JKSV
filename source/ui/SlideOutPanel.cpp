#include "ui/SlideOutPanel.hpp"

#include "config/config.hpp"
#include "graphics/colors.hpp"
#include "mathutil.hpp"

#include <cmath>
#include <utility>

namespace
{
    constexpr int SCREEN_WIDTH = 1280;
}

ui::SlideOutPanel::SlideOutPanel(int width, Side side)
    : m_x(side == Side::Left ? static_cast<double>(-width) : static_cast<double>(SCREEN_WIDTH))
    , m_width(width)
    , m_targetX(side == Side::Left ? 0.0f : static_cast<double>(SCREEN_WIDTH) - m_width)
    , m_side(side)
{
    static int targetID    = 0;
    std::string targetName = "panelTarget_" + std::to_string(targetID++);
    m_renderTarget         = sdl::TextureManager::load(targetName, width, 720, SDL_TEXTUREACCESS_TARGET);
}

void ui::SlideOutPanel::update(bool hasFocus)
{
    if (hasFocus && SlideOutPanel::is_hidden()) { SlideOutPanel::unhide(); }

    slide_out();

    if (m_isOpen)
    {
        for (auto &currentElement : m_elements) { currentElement->update(hasFocus); }
    }
}

void ui::SlideOutPanel::sub_update() { SlideOutPanel::close_hide_panel(); }

void ui::SlideOutPanel::render(sdl::SharedTexture &target, bool hasFocus)
{
    for (auto &currentElement : m_elements) { currentElement->render(m_renderTarget, hasFocus); }

    m_renderTarget->render(target, m_x, 0);
}

void ui::SlideOutPanel::clear_target() { m_renderTarget->clear(colors::SLIDE_PANEL_CLEAR); }

void ui::SlideOutPanel::reset()
{
    m_x          = m_side == Side::Left ? -(m_width) : SCREEN_WIDTH;
    m_isOpen     = false;
    m_closePanel = false;
}

void ui::SlideOutPanel::close() { m_closePanel = true; }

void ui::SlideOutPanel::hide() { m_hidePanel = true; }

void ui::SlideOutPanel::unhide() { m_hidePanel = false; }

bool ui::SlideOutPanel::is_open() const { return m_isOpen; }

bool ui::SlideOutPanel::is_closed()
{
    close_hide_panel();
    const bool closed = m_side == Side::Left ? m_x <= -m_width : m_x >= SCREEN_WIDTH;
    return m_closePanel && closed;
}

bool ui::SlideOutPanel::is_hidden() const { return m_hidePanel; }

void ui::SlideOutPanel::push_new_element(std::shared_ptr<ui::Element> newElement) { m_elements.push_back(newElement); }

void ui::SlideOutPanel::clear_elements() { m_elements.clear(); }

sdl::SharedTexture &ui::SlideOutPanel::get_target() { return m_renderTarget; }

void ui::SlideOutPanel::slide_out()
{
    const bool needsSlideOut = !m_isOpen && !m_closePanel && !m_hidePanel;
    if (!needsSlideOut) { return; }

    const bool slideRight = m_side == SlideOutPanel::Side::Left && m_x < 0;
    const bool slideLeft  = m_side == SlideOutPanel::Side::Right && m_x > SCREEN_WIDTH - m_width;

    if (slideRight) { SlideOutPanel::slide_out_left(); }
    else if (slideLeft) { SlideOutPanel::slide_out_right(); }
}

void ui::SlideOutPanel::slide_out_left()
{
    const double scaling = config::get_animation_scaling();

    m_x -= std::round(m_x / scaling);

    // This is a workaround for the floating points never lining up quite right.
    const int distance = math::Util<double>::absolute_distance(m_x, m_targetX);
    if (distance <= 2)
    {
        m_x      = m_targetX;
        m_isOpen = true;
    }
}

void ui::SlideOutPanel::slide_out_right()
{
    const double scaling     = config::get_animation_scaling();
    const double screenWidth = static_cast<double>(SCREEN_WIDTH);
    const double width       = static_cast<double>(m_width);
    const double pixels      = (screenWidth - width - m_x) / scaling;
    m_x += std::round(pixels);

    const int distance = math::Util<double>::absolute_distance(m_x, m_targetX);
    if (distance <= 2)
    {
        m_x      = m_targetX;
        m_isOpen = true;
    }
}

void ui::SlideOutPanel::close_hide_panel()
{
    const double scaling    = config::get_animation_scaling();
    const bool closeHide    = m_closePanel || m_hidePanel;
    const bool closeToLeft  = closeHide && m_side == Side::Left && m_x > -m_width;
    const bool closeToRight = closeHide && m_side == Side::Right && m_x < SCREEN_WIDTH;
    if (closeToLeft) { m_x += -((m_width - m_x) / scaling); }
    else if (closeToRight) { m_x += m_x / scaling; }

    const bool closedOrHidden = (m_x <= -m_width) || (m_x >= SCREEN_WIDTH);
    if (closedOrHidden) { m_isOpen = false; }
}

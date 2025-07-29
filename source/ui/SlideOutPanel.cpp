#include "ui/SlideOutPanel.hpp"

#include "colors.hpp"
#include "config.hpp"

#include <cmath>

namespace
{
    constexpr int SCREEN_WIDTH = 1280;
}

ui::SlideOutPanel::SlideOutPanel(int width, Side side)
    : m_x(side == Side::Left ? -width : SCREEN_WIDTH)
    , m_width(width)
    , m_targetX(side == Side::Left ? 0 : SCREEN_WIDTH - m_width)
    , m_side(side)
{
    static int slidePanelTargetID = 0;
    std::string panelTargetName   = "PanelTarget_" + std::to_string(slidePanelTargetID++);
    m_renderTarget                = sdl::TextureManager::create_load_texture(panelTargetName,
                                                              width,
                                                              720,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
}

void ui::SlideOutPanel::update(bool hasFocus)
{
    const double scaling        = config::get_animation_scaling();
    const bool openingFromLeft  = !m_isOpen && m_side == Side::Left && m_x < m_targetX;
    const bool openingFromRight = !m_isOpen && m_side == Side::Right && m_x > m_targetX;
    if (openingFromLeft) { m_x -= std::round(m_x / scaling); }
    else if (openingFromRight)
    {
        const double screenWidth = static_cast<double>(SCREEN_WIDTH);
        const double width       = static_cast<double>(m_width);
        const double pixels      = (screenWidth - width - m_x) / scaling;
        m_x += std::round(pixels);
    }
    else { m_isOpen = true; }

    // I'm going to leave it to the individual elements whether they update if the state is active.
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

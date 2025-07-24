#include "ui/SlideOutPanel.hpp"

#include "colors.hpp"
#include "config.hpp"

#include <cmath>

ui::SlideOutPanel::SlideOutPanel(int width, Side side)
    : m_x(side == Side::Left ? -width : 1280)
    , m_width(width)
    , m_targetX(side == Side::Left ? 0 : 1280 - m_width)
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
    double scaling = config::get_animation_scaling();

    // The first two conditions are just a workaround because my math keeps leaving two pixels.
    if (!m_isOpen && m_side == Side::Left && m_x >= -4)
    {
        m_x      = 0;
        m_isOpen = true;
    }
    else if (!m_isOpen && m_side == Side::Right && m_x - m_targetX <= 4)
    {
        m_x      = 1280 - m_width;
        m_isOpen = true;
    }
    else if (!m_isOpen && m_side == Side::Left && m_x != m_targetX) { m_x -= std::ceil(m_x / scaling); }
    else if (!m_isOpen && m_side == Side::Right && m_x != m_targetX)
    {
        m_x += std::ceil((1280.0f - (static_cast<double>(m_width)) - m_x) / scaling);
    }

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
    m_x          = m_side == Side::Left ? -(m_width) : 1280.0f;
    m_isOpen     = false;
    m_closePanel = false;
}

void ui::SlideOutPanel::close() { m_closePanel = true; }

bool ui::SlideOutPanel::is_open() const { return m_isOpen; }

bool ui::SlideOutPanel::is_closed() const { return m_closePanel && (m_side == Side::Left ? m_x > -(m_width) : m_x < 1280); }

void ui::SlideOutPanel::push_new_element(std::shared_ptr<ui::Element> newElement) { m_elements.push_back(newElement); }

void ui::SlideOutPanel::clear_elements() { m_elements.clear(); }

SDL_Texture *ui::SlideOutPanel::get_target() { return m_renderTarget->get(); }

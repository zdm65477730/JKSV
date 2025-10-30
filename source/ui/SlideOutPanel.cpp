#include "ui/SlideOutPanel.hpp"

#include "config/config.hpp"
#include "graphics/colors.hpp"
#include "graphics/screen.hpp"
#include "mathutil.hpp"

#include <cmath>
#include <utility>

//                      ---- Construction ----

ui::SlideOutPanel::SlideOutPanel(int width, Side side)
    : m_width(width)
    , m_targetX(side == Side::Left ? 0.0f : static_cast<double>(graphics::SCREEN_WIDTH) - m_width)
    , m_side(side)
    , m_transition(m_side == Side::Left ? -m_width : graphics::SCREEN_WIDTH,
                   0,
                   0,
                   0,
                   m_targetX,
                   0,
                   0,
                   0,
                   ui::Transition::DEFAULT_THRESHOLD)
    , m_renderTarget(sdl::TextureManager::load("PANEL_" + std::to_string(sm_targetID++),
                                               m_width,
                                               graphics::SCREEN_HEIGHT,
                                               SDL_TEXTUREACCESS_TARGET)) {};

//                      ---- Public functions ----

void ui::SlideOutPanel::update(bool hasFocus)
{
    const int targetX = m_transition.get_target_x();
    if (hasFocus && targetX != m_targetX) { SlideOutPanel::unhide(); }

    m_transition.update();

    if (m_transition.in_place())
    {
        for (auto &currentElement : m_elements) { currentElement->update(hasFocus); }
    }
}

void ui::SlideOutPanel::sub_update() { m_transition.update(); }

void ui::SlideOutPanel::render(sdl::SharedTexture &target, bool hasFocus)
{
    for (auto &currentElement : m_elements) { currentElement->render(m_renderTarget, hasFocus); }

    const int x = m_transition.get_x();
    m_renderTarget->render(target, x, 0);
}

void ui::SlideOutPanel::clear_target() { m_renderTarget->clear(colors::SLIDE_PANEL_CLEAR); }

void ui::SlideOutPanel::reset() noexcept
{
    const int x = SlideOutPanel::get_target_close_x();
    m_transition.set_x(x);
    m_isOpen     = false;
    m_closePanel = false;
}

void ui::SlideOutPanel::close() noexcept
{
    const int targetX = SlideOutPanel::get_target_close_x();
    m_transition.set_target_x(targetX);
    m_closePanel = true;
}

void ui::SlideOutPanel::hide() noexcept
{
    const int targetX = SlideOutPanel::get_target_close_x();
    m_transition.set_target_x(targetX);
    m_hidePanel = true;
}

void ui::SlideOutPanel::unhide() noexcept
{
    const int targetX = SlideOutPanel::get_target_open_x();
    m_transition.set_target_x(targetX);
    m_hidePanel = false;
}

bool ui::SlideOutPanel::is_open() const noexcept { return m_transition.in_place(); }

bool ui::SlideOutPanel::is_closed() noexcept { return m_closePanel && m_transition.in_place(); }

bool ui::SlideOutPanel::is_hidden() const noexcept { return m_hidePanel; }

void ui::SlideOutPanel::push_new_element(std::shared_ptr<ui::Element> newElement) { m_elements.push_back(newElement); }

void ui::SlideOutPanel::clear_elements() { m_elements.clear(); }

sdl::SharedTexture &ui::SlideOutPanel::get_target() noexcept { return m_renderTarget; }

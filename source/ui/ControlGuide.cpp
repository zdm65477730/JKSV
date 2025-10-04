#include "ui/ControlGuide.hpp"

#include "graphics/colors.hpp"
#include "logging/logger.hpp"

ui::ControlGuide::ControlGuide(const char *guide)
    : m_guide(guide)
    , m_textWidth(sdl::text::get_width(23, m_guide))
    , m_targetX(1220 - (m_textWidth + 24))
    , m_guideWidth(1280 - m_targetX)
    , m_transition(1280, 662, 0, 0, m_targetX, 662, 0, 0, ui::Transition::DEFAULT_THRESHOLD)
{
    ui::ControlGuide::initialize_static_members();
}

void ui::ControlGuide::update(bool hasFocus)
{
    m_transition.update();
    if (!m_transition.in_place()) { return; }

    const int targetX = m_transition.get_target_x();
    if (hasFocus && targetX != m_targetX) { m_transition.set_target_x(m_targetX); }
}

void ui::ControlGuide::sub_update()
{
    const int targetX = m_transition.get_target_x();
    if (targetX != 1280) { m_transition.set_target_x(1280); }

    m_transition.update();
}

void ui::ControlGuide::render(sdl::SharedTexture &target, bool hasFocus)
{
    const int guideX = m_transition.get_x();
    const int guideY = m_transition.get_y();

    sm_controlCap->render(sdl::Texture::Null, guideX, guideY);
    sdl::render_rect_fill(sdl::Texture::Null, guideX + 16, guideY, m_guideWidth - 16, 48, colors::GUIDE_COLOR);
    sdl::text::render(sdl::Texture::Null, guideX + 24, guideY + 10, 23, sdl::text::NO_WRAP, colors::WHITE, m_guide);
}

void ui::ControlGuide::initialize_static_members()
{
    static constexpr std::string_view NAME_CAP = "ControlGuideCap";

    if (sm_controlCap) { return; }

    sm_controlCap = sdl::TextureManager::load(NAME_CAP, "romfs:/Textures/GuideCap.png");
}

void ui::ControlGuide::reset() noexcept
{
    m_transition.set_target_x(1280);
    m_transition.set_x(1280);
}

#include "ui/ControlGuide.hpp"

#include "graphics/colors.hpp"
#include "graphics/screen.hpp"
#include "logging/logger.hpp"

namespace
{
    /// @brief This is the X coordinate used to calculate where the guide should be.
    constexpr int GUIDE_X_OFFSET = 1220;

    /// @brief This is the font size used for the guides.
    constexpr int GUIDE_TEXT_SIZE = 24;

    /// @brief This is the constant Y of the transition.
    constexpr int TRANS_Y = 662;
}

//                      ---- Construction ----

ui::ControlGuide::ControlGuide(const char *guide)
    : m_guide(guide)
    , m_textWidth(sdl::text::get_width(GUIDE_TEXT_SIZE, m_guide))
    , m_targetX(GUIDE_X_OFFSET - (m_textWidth + 24))
    , m_guideWidth(graphics::SCREEN_WIDTH - m_targetX)
    , m_transition(graphics::SCREEN_WIDTH, TRANS_Y, 0, 0, m_targetX, TRANS_Y, 0, 0, ui::Transition::DEFAULT_THRESHOLD)
{
    ui::ControlGuide::initialize_static_members();
}

//                      ---- Public functions ----

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
    if (targetX != graphics::SCREEN_WIDTH) { m_transition.set_target_x(graphics::SCREEN_WIDTH); }

    m_transition.update();
}

void ui::ControlGuide::render(sdl::SharedTexture &target, bool hasFocus)
{
    static constexpr int RECT_OFFSET_X = 16;
    static constexpr int RECT_HEIGHT   = 48;

    static constexpr int TEXT_OFFSET_X  = 24;
    static constexpr int TEXT_OFFSET_Y  = 10;
    static constexpr int TEXT_FONT_SIZE = 24;

    const int guideX = m_transition.get_x();
    const int guideY = m_transition.get_y();

    sm_controlCap->render(sdl::Texture::Null, guideX, guideY);
    sdl::render_rect_fill(sdl::Texture::Null,
                          guideX + RECT_OFFSET_X,
                          guideY,
                          m_guideWidth - RECT_OFFSET_X,
                          RECT_HEIGHT,
                          colors::GUIDE_COLOR);
    sdl::text::render(sdl::Texture::Null,
                      guideX + TEXT_OFFSET_X,
                      guideY + TEXT_OFFSET_Y,
                      TEXT_FONT_SIZE,
                      sdl::text::NO_WRAP,
                      colors::WHITE,
                      m_guide);
}

//                      ---- Private functions ----

void ui::ControlGuide::initialize_static_members()
{
    static constexpr std::string_view NAME_CAP = "ControlGuideCap";

    if (sm_controlCap) { return; }

    sm_controlCap = sdl::TextureManager::load(NAME_CAP, "romfs:/Textures/GuideCap.png");
}

void ui::ControlGuide::reset() noexcept
{
    m_transition.set_target_x(graphics::SCREEN_WIDTH);
    m_transition.set_x(graphics::SCREEN_WIDTH);
}

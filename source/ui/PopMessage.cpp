#include "ui/PopMessage.hpp"

#include "config/config.hpp"
#include "graphics/colors.hpp"
#include "graphics/screen.hpp"
#include "mathutil.hpp"
#include "sdl.hpp"

namespace
{
    constexpr int START_X              = 624;
    constexpr int START_WIDTH          = 20;
    constexpr int PERMA_HEIGHT         = 48;
    constexpr int TRANSITION_THRESHOLD = 2; // This is used so the easing on opening doesn't look so odd.
}

//                      ---- Construction ----

ui::PopMessage::PopMessage(int ticks, std::string_view message)
    : m_transition(START_X, graphics::SCREEN_HEIGHT, START_WIDTH, PERMA_HEIGHT, 0, 0, 0, 0, TRANSITION_THRESHOLD)
    , m_ticks(ticks)
    , m_message(message)
{
    PopMessage::initialize_static_members();
}

ui::PopMessage::PopMessage(int ticks, std::string &message)
    : m_transition(START_X, graphics::SCREEN_HEIGHT, START_WIDTH, PERMA_HEIGHT, 0, 0, 0, 0, TRANSITION_THRESHOLD)
    , m_ticks(ticks)
    , m_message(std::move(message))
{
    PopMessage::initialize_static_members();
}

//                      ---- Public functions ----

void ui::PopMessage::update(double targetY)
{
    PopMessage::update_y(targetY);
    PopMessage::update_text_offset();
    PopMessage::update_width();

    if (m_close && m_transition.in_place()) { m_finished = true; }
    else if (m_displayTimer.is_triggered()) { PopMessage::close(); }
}

void ui::PopMessage::render()
{
    PopMessage::render_container();
    if (!m_yMet || m_close) { return; }

    // This avoids allocating and returning another std::string.
    const int y = m_transition.get_y();
    const std::string_view message(m_message.c_str(), m_substrOffset);
    sdl::text::render(sdl::Texture::Null, m_textX, y + 11, 22, sdl::text::NO_WRAP, colors::BLACK, message);
}

bool ui::PopMessage::finished() const noexcept { return m_finished; }

std::string_view ui::PopMessage::get_message() const noexcept { return m_message; }

//                      ---- Private functions ----

void ui::PopMessage::initialize_static_members()
{
    static constexpr std::string_view TEX_CAP_NAME = "PopCaps";
    static constexpr const char *TEX_CAP_PATH      = "romfs:/Textures/PopMessage.png";

    if (sm_endCaps) { return; }

    sm_endCaps = sdl::TextureManager::load(TEX_CAP_NAME, TEX_CAP_PATH);
}

void ui::PopMessage::update_y(double targetY) noexcept
{
    if (!m_close) { m_transition.set_target_y(targetY); };

    m_transition.update_xy();
    if (!m_yMet && m_transition.in_place_xy())
    {
        m_yMet = true;
        m_typeTimer.start(5);
    }
}

void ui::PopMessage::update_text_offset()
{
    static constexpr int HALF_WIDTH = 640;

    const int messageLength = m_message.length();
    if (!m_yMet || m_substrOffset >= messageLength || !m_typeTimer.is_triggered()) { return; }

    // This is slightly more technical than I originally thought, but I liked the effect so.
    uint32_t codepoint{};
    const char *message        = m_message.c_str();
    const uint8_t *targetPoint = reinterpret_cast<const uint8_t *>(&message[m_substrOffset]);
    const ssize_t unitCount    = decode_utf8(&codepoint, targetPoint);
    if (unitCount <= 0) { return; }

    m_substrOffset += unitCount;
    const std::string_view subMessage(message, m_substrOffset);
    const int stringWidth = sdl::text::get_width(22, subMessage);
    m_textX               = HALF_WIDTH - (stringWidth / 2);

    const int dialogWidth = stringWidth + 32;
    m_transition.set_target_width(dialogWidth);
    if (m_substrOffset >= messageLength) { m_displayTimer.start(m_ticks); }
}

void ui::PopMessage::update_width() noexcept
{
    if (!m_yMet) { return; }

    m_transition.update_width_height();

    if (m_close && m_transition.in_place_width_height()) { m_transition.set_target_y(graphics::SCREEN_HEIGHT); }
}

void ui::PopMessage::render_container() noexcept
{
    constexpr int WIDTH = 48;
    constexpr int HALF  = WIDTH / 2;

    const int x     = m_transition.get_centered_x() - (HALF / 2);
    const int y     = m_transition.get_y();
    const int width = m_transition.get_width() + HALF;

    sm_endCaps->render_part(sdl::Texture::Null, x, y, 0, 0, HALF, WIDTH);
    sdl::render_rect_fill(sdl::Texture::Null, x + HALF, y, width - WIDTH, 48, colors::DIALOG_LIGHT);
    sm_endCaps->render_part(sdl::Texture::Null, x + (width - HALF), y, HALF, 0, HALF, 48);
}

void ui::PopMessage::close() noexcept
{
    m_close = true;
    m_transition.set_target_width(START_WIDTH);
}
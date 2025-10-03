#include "ui/PopMessage.hpp"

#include "config/config.hpp"
#include "graphics/colors.hpp"
#include "mathutil.hpp"
#include "sdl.hpp"

namespace
{
    static inline constexpr int START_X      = 624;
    static inline constexpr double START_Y   = 720;
    static inline constexpr int START_WIDTH  = 32;
    static inline constexpr int PERMA_HEIGHT = 48;
}

ui::PopMessage::PopMessage(int ticks, std::string_view message)
    : m_transition(START_X, START_Y, START_WIDTH, PERMA_HEIGHT, 0, 0, 0, 0, 4)
    , m_ticks(ticks)
    , m_message(message)
    , m_dialog(ui::DialogBox::create(START_X, START_Y - 6, START_WIDTH, PERMA_HEIGHT, ui::DialogBox::Type::Light)) {};

ui::PopMessage::PopMessage(int ticks, std::string &message)
    : m_transition(START_X, START_Y, START_WIDTH, PERMA_HEIGHT, 0, 0, 0, 0, 4)
    , m_ticks(ticks)
    , m_message(std::move(message))
    , m_dialog(ui::DialogBox::create(START_X, START_Y - 6, START_WIDTH, PERMA_HEIGHT, ui::DialogBox::Type::Light)) {};

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
    m_dialog->render(sdl::Texture::Null, false);
    if (!m_yMet || m_close) { return; }

    // This avoids allocating and returning another std::string.
    const int y = m_transition.get_y();
    const std::string_view message(m_message.c_str(), m_substrOffset);
    sdl::text::render(sdl::Texture::Null, m_textX, y + 5, 22, sdl::text::NO_WRAP, colors::BLACK, message);
}

bool ui::PopMessage::finished() const noexcept { return m_finished; }

std::string_view ui::PopMessage::get_message() const noexcept { return m_message; }

void ui::PopMessage::update_y(double targetY) noexcept
{
    if (!m_close) { m_transition.set_target_y(targetY); };

    m_transition.update_xy();
    if (!m_yMet && m_transition.in_place_xy())
    {
        m_yMet = true;
        m_typeTimer.start(5);
    }

    const int y = m_transition.get_y();
    m_dialog->set_y(y - 6);
}

void ui::PopMessage::update_text_offset() noexcept
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

    const int dialogX     = m_textX - 16;
    const int dialogWidth = stringWidth + 32;

    m_transition.set_target_x(dialogX);
    m_transition.set_target_width(dialogWidth);
    if (m_substrOffset >= messageLength) { m_displayTimer.start(m_ticks); }
}

void ui::PopMessage::update_width() noexcept
{
    if (!m_yMet) { return; }

    m_transition.update_width_height();

    const int x     = m_transition.get_centered_x();
    const int width = m_transition.get_width();

    m_dialog->set_x(x);
    m_dialog->set_width(width);

    if (m_close && m_transition.in_place_width_height()) { m_transition.set_target_y(720); }
}

void ui::PopMessage::close() noexcept
{
    m_close = true;
    m_transition.set_target_width(32);
}
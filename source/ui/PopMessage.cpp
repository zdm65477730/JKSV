#include "ui/PopMessage.hpp"

#include "config/config.hpp"
#include "graphics/colors.hpp"
#include "mathutil.hpp"
#include "sdl.hpp"

ui::PopMessage::PopMessage(int ticks, std::string_view message)
    : m_ticks(ticks)
    , m_message(message)
    , m_y(PopMessage::START_Y)
    , m_width(PopMessage::START_WIDTH)
    , m_dialog(ui::DialogBox::create(PopMessage::START_X,
                                     m_y - 6,
                                     PopMessage::START_WIDTH,
                                     PopMessage::PERMA_HEIGHT,
                                     ui::DialogBox::Type::Light)) {};

ui::PopMessage::PopMessage(int ticks, std::string &message)
    : m_ticks(ticks)
    , m_message(std::move(message))
    , m_y(PopMessage::START_Y)
    , m_width(PopMessage::START_WIDTH)
    , m_dialog(ui::DialogBox::create(PopMessage::START_X,
                                     m_y - 6,
                                     PopMessage::START_WIDTH,
                                     PopMessage::PERMA_HEIGHT,
                                     ui::DialogBox::Type::Light)) {};

void ui::PopMessage::update(double targetY)
{
    update_y(targetY);
    update_text_offset();
    if (m_displayTimer.is_triggered()) { m_finished = true; }
}

void ui::PopMessage::render()
{
    m_dialog->render(sdl::Texture::Null, false);
    if (!m_yMet) { return; }
    // This avoids allocating and returning another std::string.
    const std::string_view message(m_message.c_str(), m_substrOffset);
    sdl::text::render(sdl::Texture::Null, m_textX, m_y + 5, 22, sdl::text::NO_WRAP, colors::BLACK, message);
}

bool ui::PopMessage::finished() const noexcept { return m_finished; }

std::string_view ui::PopMessage::get_message() const noexcept { return m_message; }

void ui::PopMessage::update_y(double targetY) noexcept
{
    if (m_y == targetY) { return; }

    const double scaling  = config::get_animation_scaling();
    const double increase = (targetY - m_y) / scaling;
    m_y += increase;

    const int distance = math::Util<double>::absolute_distance(targetY, m_y);
    if (distance <= 2)
    {
        m_y    = targetY;
        m_yMet = true;
        m_typeTimer.start(5);
    }
    m_dialog->set_y(m_y - 6);
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

    m_dialog->set_x(dialogX);
    m_dialog->set_width(dialogWidth);
    if (m_substrOffset >= messageLength) { m_displayTimer.start(m_ticks); }
}

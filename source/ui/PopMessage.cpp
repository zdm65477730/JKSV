#include "ui/PopMessage.hpp"

#include "colors.hpp"
#include "config.hpp"
#include "mathutil.hpp"
#include "sdl.hpp"

ui::PopMessage::PopMessage(int ticks, std::string_view message)
    : m_ticks(ticks)
    , m_message(message)
    , m_y(PopMessage::START_Y)
    , m_width(PopMessage::START_WIDTH)
    , m_dialog(ui::DialogBox::create(PopMessage::PERMA_X, m_y - 6, PopMessage::START_WIDTH, PopMessage::PERMA_HEIGHT))
{
    const int messageWidth = sdl::text::get_width(22, m_message);
    m_targetWidth          = messageWidth + 32;
}

void ui::PopMessage::update(double targetY)
{
    update_y(targetY);
    update_width();
    update_text_offset();
    if (m_displayTimer.is_triggered()) { m_finished = true; }
}

void ui::PopMessage::render()
{
    m_dialog->render(sdl::Texture::Null, false);
    if (!m_expanded) { return; }
    sdl::text::render(sdl::Texture::Null,
                      PopMessage::PERMA_X + 16,
                      m_y + 4,
                      22,
                      sdl::text::NO_WRAP,
                      colors::WHITE,
                      m_message.substr(0, m_substrOffset));
}

bool ui::PopMessage::finished() const { return m_finished; }

void ui::PopMessage::update_y(double targetY)
{
    if (m_y == targetY) { return; }

    const double scaling  = config::get_animation_scaling();
    const double increase = (targetY - m_y) / scaling;
    m_y += increase;
    m_dialog->set_xy(m_dialog->NO_SET, m_y - 6);
}

void ui::PopMessage::update_width()
{
    if (m_expanded) { return; }

    const int gap = math::Util<int>::absolute_distance(m_targetWidth, m_width);
    if (gap <= 4)
    {
        m_typeTimer.start(5);
        m_width    = m_targetWidth;
        m_expanded = true;
        m_dialog->set_width_height(m_width, m_dialog->NO_SET);
        return;
    }

    const double scaling = config::get_animation_scaling();
    const int expand     = static_cast<double>(gap) / scaling;
    m_width += expand;
    m_dialog->set_width_height(m_width, m_dialog->NO_SET);
}

void ui::PopMessage::update_text_offset()
{
    const int messageLength = m_message.length();
    if (!m_expanded || m_substrOffset >= messageLength) { return; }

    // This is slightly more technical than I originally thought, but I liked the effect so.
    uint32_t codepoint{};
    const char *message        = m_message.c_str();
    const uint8_t *targetPoint = reinterpret_cast<const uint8_t *>(&message[m_substrOffset]);
    const ssize_t unitCount    = decode_utf8(&codepoint, targetPoint);
    if (unitCount <= 0) { return; }

    m_substrOffset += unitCount;
    if (m_substrOffset >= messageLength) { m_displayTimer.start(m_ticks); }
}

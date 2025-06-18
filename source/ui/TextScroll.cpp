#include "ui/TextScroll.hpp"
#include "sdl.hpp"

namespace
{
    /// @brief This is the number of ticks needed before the text starts scrolling.
    constexpr uint64_t TICKS_SCROLL_TRIGGER = 3000;

    /// @brief This is the number of pixels between the two renderings of the text.
    constexpr int SIZE_TEXT_GAP = 0;
} // namespace

ui::TextScroll::TextScroll(std::string_view text,
                           int fontSize,
                           int availableWidth,
                           int y,
                           bool center,
                           sdl::Color color)
{
    TextScroll::create(text, fontSize, availableWidth, y, center, color);
}

void ui::TextScroll::create(std::string_view text,
                            int fontSize,
                            int availableWidth,
                            int y,
                            bool center,
                            sdl::Color color)
{
    // Copy the text and stuff.
    m_text = text;
    m_y = y;
    m_fontSize = fontSize;
    m_textColor = color;
    m_scrollTimer.start(TICKS_SCROLL_TRIGGER);

    // Get the width and calculate X.
    m_textWidth = sdl::text::get_width(m_fontSize, m_text.c_str());
    if (m_textWidth > availableWidth)
    {
        // Set the X coordinate to 8 and make sure this knows it needs to scroll.
        m_x = 8;
        m_textScrolling = true;
    }
    else if (center)
    {
        // Just center it.
        m_x = (availableWidth / 2) - (m_textWidth / 2);
    }
    else
    {
        // Just set this to 8. To do: Figure out how to make this cleaner.
        m_x = 8;
    }
}

void ui::TextScroll::update(bool hasFocus)
{
    // I don't think needs to care about having focus.
    if (m_textScrolling && m_scrollTimer.is_triggered())
    {
        m_x -= 2;
        m_textScrollTriggered = true;
    }
    else if (m_textScrollTriggered && m_x > -(m_textWidth + SIZE_TEXT_GAP))
    {
        m_x -= 2;
    }
    else if (m_textScrollTriggered && m_x <= -(m_textWidth + SIZE_TEXT_GAP))
    {
        // This will snap the text back to where it was, but the user won't even notice it. It just looks like it's scrolling.
        m_x = 8;
        m_textScrollTriggered = false;
        m_scrollTimer.restart();
    }
}

void ui::TextScroll::render(SDL_Texture *target, bool hasFocus)
{
    // If we don't need to scroll the text, just render it as-is centered according to the width passed earlier.
    if (!m_textScrolling)
    {
        sdl::text::render(target, m_x, m_y, m_fontSize, sdl::text::NO_TEXT_WRAP, m_textColor, m_text.c_str());
    }
    else
    {
        // We're going to render text twice so it looks like it's scrolling and doesn't end. Ever.
        sdl::text::render(target, m_x, m_y, m_fontSize, sdl::text::NO_TEXT_WRAP, m_textColor, m_text.c_str());
        sdl::text::render(target,
                          m_x + m_textWidth + 8,
                          m_y,
                          m_fontSize,
                          sdl::text::NO_TEXT_WRAP,
                          m_textColor,
                          m_text.c_str());
    }
}

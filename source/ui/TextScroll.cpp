#include "ui/TextScroll.hpp"
#include "sdl.hpp"

ui::TextScroll::TextScroll(std::string_view text, int fontSize, int availableWidth, int y, sdl::Color color)
    : m_text(text.data()), m_y(y), m_fontSize(fontSize), m_textColor(color), m_scrollTimer(3000)
{
    // Grab text width first.
    m_textWidth = sdl::text::getWidth(m_fontSize, m_text.c_str());

    // Check if text needs scrolling for width provided.
    if (m_textWidth > availableWidth)
    {
        m_x = 8;
        m_textScrolling = true;
    }
    else
    {
        // Just center it.
        m_x = (availableWidth / 2) - (m_textWidth / 2);
    }
}

ui::TextScroll &ui::TextScroll::operator=(const ui::TextScroll &textScroll)
{
    m_text = textScroll.m_text;
    m_x = textScroll.m_x;
    m_y = textScroll.m_y;
    m_fontSize = textScroll.m_fontSize;
    m_textColor = textScroll.m_textColor;
    m_textWidth = textScroll.m_textWidth;
    m_textScrolling = textScroll.m_textScrolling;
    m_textScrollTriggered = textScroll.m_textScrollTriggered;
    m_scrollTimer = textScroll.m_scrollTimer;
    return *this;
}

void ui::TextScroll::update(bool hasFocus)
{
    // I don't think needs to care about having focus.
    if (m_textScrolling && m_scrollTimer.isTriggered())
    {
        m_x -= 2;
        m_textScrollTriggered = true;
    }
    else if (m_textScrollTriggered && m_x > -(m_textWidth + 16))
    {
        m_x -= 2;
    }
    else if (m_textScrollTriggered && m_x <= -(m_textWidth + 16))
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
        sdl::text::render(target, m_x + m_textWidth + 24, m_y, m_fontSize, sdl::text::NO_TEXT_WRAP, m_textColor, m_text.c_str());
    }
}

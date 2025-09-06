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
                           int x,
                           int y,
                           int width,
                           int height,
                           int fontSize,
                           sdl::Color textColor,
                           sdl::Color clearColor,
                           bool center)
{
    TextScroll::initialize(text, x, y, width, height, fontSize, textColor, clearColor, center);
}

void ui::TextScroll::initialize(std::string_view text,
                                int x,
                                int y,
                                int width,
                                int height,
                                int fontSize,
                                sdl::Color textColor,
                                sdl::Color clearColor,
                                bool center)
{
    static int TARGET_ID{};

    m_renderX      = x;
    m_renderY      = y;
    m_fontSize     = fontSize;
    m_textColor    = textColor;
    m_clearColor   = clearColor;
    m_targetWidth  = width;
    m_targetHeight = height;
    m_textY        = (m_targetHeight / 2) - (m_fontSize / 2);
    m_scrollTimer.start(TICKS_SCROLL_TRIGGER);

    {
        const std::string targetName = "textScroll_" + std::to_string(TARGET_ID++);
        m_renderTarget = sdl::TextureManager::load(targetName, m_targetWidth, m_targetHeight, SDL_TEXTUREACCESS_TARGET);
    }

    TextScroll::set_text(text, center);
}

std::string_view ui::TextScroll::get_text() const noexcept { return m_text; }

void ui::TextScroll::set_text(std::string_view text, bool center)
{
    m_text                = text;
    m_textWidth           = sdl::text::get_width(m_fontSize, m_text.c_str());
    m_textScrollTriggered = false;

    if (m_textWidth > m_targetWidth - 16)
    {
        m_textX         = 8;
        m_textScrolling = true;
        m_scrollTimer.restart();
    }
    else if (center)
    {
        m_textX         = (m_targetWidth / 2) - (m_textWidth / 2);
        m_textScrolling = false;
    }
    else
    {
        m_textX         = 8;
        m_textScrolling = false;
    }
}

void ui::TextScroll::update(bool hasFocus)
{
    // I don't think this needs to care about having focus.
    const int invertedWidth    = -(m_textWidth + SIZE_TEXT_GAP);
    const bool scrollTriggered = hasFocus && m_textScrolling && m_scrollTimer.is_triggered();
    const bool keepScrolling   = m_textScrollTriggered && m_textX > invertedWidth;
    const bool scrollFinished  = m_textScrollTriggered && m_textX <= invertedWidth;

    if (scrollTriggered)
    {
        m_textX -= 2;
        m_textScrollTriggered = true;
    }
    else if (keepScrolling) { m_textX -= 2; }
    else if (scrollFinished)
    {
        // This will snap the text back. You can't see it though.
        m_textX               = 8;
        m_textScrollTriggered = false;
        m_scrollTimer.restart();
    }
}

void ui::TextScroll::render(sdl::SharedTexture &target, bool hasFocus)
{
    m_renderTarget->clear(m_clearColor);

    if (!m_textScrolling)
    {
        sdl::text::render(m_renderTarget, m_textX, m_textY, m_fontSize, sdl::text::NO_WRAP, m_textColor, m_text);
    }
    else
    {
        // We're going to render text twice so it looks like it's scrolling and doesn't end. Ever.
        sdl::text::render(m_renderTarget, m_textX, m_textY, m_fontSize, sdl::text::NO_WRAP, m_textColor, m_text);
        sdl::text::render(m_renderTarget,
                          m_textX + m_textWidth + 8,
                          m_textY,
                          m_fontSize,
                          sdl::text::NO_WRAP,
                          m_textColor,
                          m_text);
    }
    m_renderTarget->render(target, m_renderX, m_renderY);
}

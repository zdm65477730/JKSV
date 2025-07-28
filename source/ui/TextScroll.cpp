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
    TextScroll::create(text, x, y, width, height, fontSize, textColor, clearColor, center);
}

void ui::TextScroll::create(std::string_view text,
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
    m_textY        = (m_targetHeight / 2) - (m_fontSize / 2) - 1; // This gets the job done, but could be better.
    m_scrollTimer.start(TICKS_SCROLL_TRIGGER);

    {
        const std::string targetName = "textScroll_" + std::to_string(TARGET_ID++);
        m_renderTarget               = sdl::TextureManager::create_load_texture(targetName,
                                                                  m_targetWidth,
                                                                  m_targetHeight,
                                                                  SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
    }

    TextScroll::set_text(text, center);
}

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

void ui::TextScroll::set_xy(int x, int y)
{
    m_renderX = x;
    m_renderY = y;
}

void ui::TextScroll::render(SDL_Texture *target, bool hasFocus)
{
    m_renderTarget->clear(m_clearColor);
    SDL_Texture *renderTarget = m_renderTarget->get();

    if (!m_textScrolling)
    {
        sdl::text::render(renderTarget, m_textX, m_textY, m_fontSize, sdl::text::NO_TEXT_WRAP, m_textColor, m_text.c_str());
    }
    else
    {
        // We're going to render text twice so it looks like it's scrolling and doesn't end. Ever.
        sdl::text::render(renderTarget, m_textX, m_textY, m_fontSize, sdl::text::NO_TEXT_WRAP, m_textColor, m_text.c_str());
        sdl::text::render(renderTarget,
                          m_textX + m_textWidth + 8,
                          m_textY,
                          m_fontSize,
                          sdl::text::NO_TEXT_WRAP,
                          m_textColor,
                          m_text.c_str());
    }
    m_renderTarget->render(target, m_renderX, m_renderY);
}

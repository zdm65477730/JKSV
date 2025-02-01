#pragma once
#include "system/Timer.hpp"
#include "ui/Element.hpp"
#include <string>

namespace ui
{
    /// @brief This is used in multiple places and rewriting it over and over is a waste of time.
    class TextScroll : public ui::Element
    {
        public:
            /// @brief This is only here so I can get around the backup menu having static members.
            TextScroll(void) = default;

            /// @brief Constructs a new TextScroll.
            /// @param text Text to be scrolled.
            /// @param availableWidth Maximum width. If this is not exceeded,text is just rendered centered according to it.
            TextScroll(std::string_view text, int fontSize, int availableWidth, int y, sdl::Color color);

            /// @brief Required destructor.
            ~TextScroll() {};

            /// @brief Same as the default constructor. To do: Figure this out better sometime.
            /// @param textScroll TextScroll to copy.
            /// @return Reference to itself?
            TextScroll &operator=(const TextScroll &textScroll);

            /// @brief Runs the update routine.
            /// @param hasFocus Whether or not the calling state has focus.
            void update(bool hasFocus);

            /// @brief Runs the render routine.
            /// @param target Target to render to.
            /// @param hasFocus Whether or not the calling state has focus.
            void render(SDL_Texture *target, bool hasFocus);

        private:
            /// @brief Text to display.
            std::string m_text;
            /// @brief X coordinate to render text at.
            int m_x;
            /// @brief Y coordinate to render text at.
            int m_y;
            /// @brief Font size used to calculate and render text.
            int m_fontSize = 0;
            /// @brief Color used to render the text.
            sdl::Color m_textColor;
            /// @brief Width of text in pixels.
            int m_textWidth = 0;
            /// @brief Whether or not text is too wide to fit into the availableWidth passed to the constructor.
            bool m_textScrolling = false;
            /// @brief Whether or not a scroll was triggered.
            bool m_textScrollTriggered = false;
            /// @brief Timer for scrolling text.
            sys::Timer m_scrollTimer;
    };
} // namespace ui

#pragma once
#include "sdl.hpp"
#include "sys/sys.hpp"
#include "ui/Element.hpp"

#include <string>

namespace ui
{
    /// @brief This is used in multiple places and rewriting it over and over is a waste of time.
    class TextScroll final : public ui::Element
    {
        public:
            /// @brief This is only here so I can get around the backup menu having static members.
            TextScroll() = default;

            /// @brief Constructor for TextScroll.
            /// @param text Text to create the textscroll with.
            /// @param fontSize Size of the font in pixels to use.
            /// @param width Maximum width to use for rendering text.
            /// @param x X coordinate to render to.
            /// @param y Y coordinate to render to.
            /// @param center Whether or not text should be centered if it's not wide enough for scrolling.
            /// @param color Color to use to render the text.
            TextScroll(std::string_view text,
                       int x,
                       int y,
                       int width,
                       int height,
                       int fontSize,
                       sdl::Color textColor,
                       sdl::Color clearColor,
                       bool center = true);

            /// @brief Creates and returns a new TextScroll. See constructor.
            static inline std::shared_ptr<ui::TextScroll> create(std::string_view text,
                                                                 int x,
                                                                 int y,
                                                                 int width,
                                                                 int height,
                                                                 int fontSize,
                                                                 sdl::Color textColor,
                                                                 sdl::Color clearColor,
                                                                 bool center = true)
            {
                return std::make_shared<ui::TextScroll>(text, x, y, width, height, fontSize, textColor, clearColor, center);
            }

            /// @brief Creates/sets the text and parameters for TextScroll.
            /// @param text Text to display/scroll.
            /// @param fontSize Font size to use when rendering.
            /// @param availableWidth Available width of the target buffer.
            /// @param y Y coordinate used to render text.
            /// @param center Whether or not text should be centered if it's not wide enough for scrolling.
            /// @param color Color to use to render text.
            void initialize(std::string_view text,
                            int x,
                            int y,
                            int width,
                            int height,
                            int fontSize,
                            sdl::Color textColor,
                            sdl::Color clearColor,
                            bool center = true);

            /// @brief Runs the update routine.
            /// @param hasFocus Whether or not the calling state has focus.
            void update(bool hasFocus) override;

            /// @brief Runs the render routine.
            /// @param target Target to render to.
            /// @param hasFocus Whether or not the calling state has focus.
            void render(sdl::SharedTexture &target, bool hasFocus) override;

            /// @brief Returns the current text being used for scrolling.
            std::string_view get_text() const noexcept;

            /// @brief Sets and allows changing the text scrolled.
            void set_text(std::string_view text, bool center);

        private:
            /// @brief Text to display.
            std::string m_text{};

            /// @brief X coordinate to render text at.
            int m_textX{};

            /// @brief Y coordinate to render text at. This is centered vertically.
            int m_textY{};

            /// @brief The X coordinate the render target is rendered to.
            int m_renderX{};

            /// @brief Y coordinate to render the target to.
            int m_renderY{};

            /// @brief Font size used to calculate and render text.
            int m_fontSize{};

            /// @brief Color used to render the text.
            sdl::Color m_textColor{};

            /// @brief Color used to clear the render target.
            sdl::Color m_clearColor{};

            /// @brief Width of text in pixels.
            int m_textWidth{};

            /// @brief Width of the render target.
            int m_targetWidth{};

            /// @brief Height of the render target.
            int m_targetHeight{};

            /// @brief Whether or not text is too wide to fit into the availableWidth passed to the constructor.
            bool m_textScrolling{};

            /// @brief Whether or not a scroll was triggered.
            bool m_textScrollTriggered{};

            sdl::SharedTexture m_renderTarget{};

            /// @brief Timer for scrolling text.
            sys::Timer m_scrollTimer;
    };
} // namespace ui

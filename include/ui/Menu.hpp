#pragma once
#include "sdl.hpp"
#include "ui/BoundingBox.hpp"
#include "ui/ColorMod.hpp"
#include "ui/Element.hpp"
#include "ui/TextScroll.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ui
{
    /// @brief Text based menu class.
    class Menu : public ui::Element
    {
        public:
            /// @brief Menu constructor.
            /// @param x X coordinate to render to.
            /// @param y Y coordinate to render to.
            /// @param width Width of the menu options in pixels.
            /// @param fontSize Size of the font in pixels to use.
            /// @param renderTargetHeight Height of the render target to calculate option height and scrolling.
            Menu(int x, int y, int width, int fontSize, int renderTargetHeight);

            /// @brief Required destructor.
            ~Menu() {};

            /// @brief Creates and returns a new ui::Menu instance.
            static inline std::shared_ptr<ui::Menu> create(int x, int y, int width, int fontSize, int renderTargetHeight)
            {
                return std::make_shared<ui::Menu>(x, y, width, fontSize, renderTargetHeight);
            }

            /// @brief Runs the update routine.
            /// @param hasFocus Whether or not the calling state has focus.
            void update(bool HasFocus) override;

            /// @brief Renders the menu.
            /// @param target Target to render to.
            /// @param hasFocus Whether or not the calling state has focus.
            void render(sdl::SharedTexture &target, bool hasFocus) override;

            /// @brief Adds and option to the menu.
            /// @param newOption Option to add to menu.
            void add_option(std::string_view newOption);

            /// @brief Allows updating and editing the option.
            /// @param newOption Option to change text to.
            void edit_option(int index, std::string_view newOption);

            /// @brief Returns the index of the currently selected menu option.
            /// @return Index of currently selected option.
            int get_selected() const;

            /// @brief Sets the selected item.
            /// @param selected Value to set selected to.
            void set_selected(int selected);

            /// @brief This is a workaround function until I find something better.
            /// @param width New width of the menu in pixels.
            void set_width(int width);

            /// @brief Resets the menu and returns it to an empty, default state.
            void reset();

        protected:
            /// @brief X coordinate menu is rendered to.
            double m_x{};

            /// @brief Y coordinate menu is rendered to.
            double m_y{};

            /// @brief Currently selected option.
            int m_selected{};

            /// @brief Height of options in pixels.
            int m_optionHeight{};

            /// @brief Target options are rendered to.
            sdl::SharedTexture m_optionTarget{};

            /// @brief Bounding box for the selected option.
            std::shared_ptr<ui::BoundingBox> m_boundingBox{};

        private:
            /// @brief This to preserve the original Y coordinate passed.
            double m_originalY{};

            /// @brief The target Y coordinate the menu should be rendered at.
            double m_targetY{};

            /// @brief How many options before scrolling happens.
            int m_scrollLength{};

            /// @brief Width of the menu in pixels.
            int m_width{};

            /// @brief Font size in pixels.
            int m_fontSize{};

            /// @brief The Y coord text is rendered to on that target.
            int m_textY{};

            /// @brief Vertical size of the destination render target in pixels.
            int m_renderTargetHeight{};

            /// @brief Maximum number of display options render target can show.
            int m_maxDisplayOptions{};

            /// @brief Vector of options.
            std::vector<std::string> m_options{};

            /// @brief Text scroll for when the current option is too long to on screen.
            std::shared_ptr<ui::TextScroll> m_optionScroll{};

            void update_scroll_text();

            void update_scrolling();

            void handle_input();
    };
} // namespace ui

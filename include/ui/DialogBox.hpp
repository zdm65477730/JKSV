#pragma once
#include "sdl.hpp"
#include "ui/Element.hpp"

#include <string>

namespace ui
{
    class DialogBox final : public ui::Element
    {
        public:
            /// @brief Creates a new dialog box instance.
            /// @param width Width of the dialog box in pixels.
            /// @param height Height of the dialog box in pixels.
            /// @param text Text to display on the dialog box.
            DialogBox(int x, int y, int width, int height);

            /// @brief Required destructor.
            ~DialogBox() {};

            /// @brief Creates and returns a new DialogBox instance.
            /// @param x X coord to render to.
            /// @param y Y coord to render to.
            /// @param width Width of the box.
            /// @param height Height of the box.
            static std::shared_ptr<ui::DialogBox> create(int x, int y, int width, int height);

            /// @brief Update override. This does NOTHING!
            void update(bool hasFocus) override {};

            /// @brief Renders the dialog box to screen.
            /// @param target Render target to render to.
            /// @param hasFocus This is ignored.
            void render(sdl::SharedTexture &target, bool hasFocus) override;

            /// @brief Sets the X and coords for the dialog box.
            void set_xy(int x, int y);

            /// @brief Sets the width and height of the dialog.
            void set_width_height(int width, int height);

            /// @brief Pass with the set functions to not change.
            static inline constexpr int NO_SET = -1;

        private:
            /// @brief X render coord.
            int m_x{};

            /// @brief Y render coord.
            int m_y{};

            /// @brief Width of the dialog.
            int m_width{};

            /// @brief Height of the dialog.
            int m_height{};

            /// @brief All instances shared this.
            static inline sdl::SharedTexture sm_corners{};

            /// @brief Initializes and loads the corners texture.
            void initialize_static_members();
    };
}

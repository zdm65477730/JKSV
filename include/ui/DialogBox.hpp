#pragma once
#include "sdl.hpp"
#include "ui/Element.hpp"

#include <string>

namespace ui
{
    class DialogBox final : public ui::Element
    {
        public:
            enum class Type
            {
                Light,
                Dark
            };

            /// @brief Creates a new dialog box instance.
            /// @param width Width of the dialog box in pixels.
            /// @param height Height of the dialog box in pixels.
            /// @param text Text to display on the dialog box.
            /// @param type Optional. The type of box. Default is dark since JKSV rewrite doesn't do theme detection.
            DialogBox(int x, int y, int width, int height, DialogBox::Type type = DialogBox::Type::Dark);

            /// @brief Required destructor.
            ~DialogBox() {};

            /// @brief Creates and returns a new DialogBox instance. See constructor.
            static inline std::shared_ptr<ui::DialogBox> create(int x,
                                                                int y,
                                                                int width,
                                                                int height,
                                                                DialogBox::Type type = DialogBox::Type::Dark)
            {
                return std::make_shared<ui::DialogBox>(x, y, width, height, type);
            }

            /// @brief Update override. This does NOTHING!
            void update(bool hasFocus) override {};

            /// @brief Renders the dialog box to screen.
            /// @param target Render target to render to.
            /// @param hasFocus This is ignored.
            void render(sdl::SharedTexture &target, bool hasFocus) override;

            /// @brief Sets the X render coord.
            void set_x(int x);

            /// @brief Sets the X render coord.
            void set_y(int y);

            /// @brief Sets the width.
            void set_width(int width);

            /// @brief Sets the height.
            void set_height(int height);

        private:
            /// @brief X render coord.
            int m_x{};

            /// @brief Y render coord.
            int m_y{};

            /// @brief Width of the dialog.
            int m_width{};

            /// @brief Height of the dialog.
            int m_height{};

            /// @brief Stores which type and corners should be used.
            DialogBox::Type m_type{};

            /// @brief All instances shared this and the other.
            static inline sdl::SharedTexture sm_darkCorners{};

            /// @brief This is the light cer
            static inline sdl::SharedTexture sm_lightCorners{};

            /// @brief Initializes and loads the corners texture.
            void initialize_static_members();
    };
}

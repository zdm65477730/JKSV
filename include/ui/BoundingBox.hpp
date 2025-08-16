#pragma once
#include "sdl.hpp"
#include "ui/ColorMod.hpp"
#include "ui/Element.hpp"

#include <memory>

namespace ui
{
    class BoundingBox final : public ui::Element
    {
        public:
            /// @brief Constructs a new bounding box instance.
            /// @param x X coord to render to.
            /// @param y Y coord to render to.
            /// @param width Width of the box in pixels.
            /// @param height Height of the box in pixels.
            BoundingBox(int x, int y, int width, int height);

            /// @brief Required destructor.
            ~BoundingBox() {};

            /// @brief Creates a returns a new BoundingBox. See constructor.
            static inline std::shared_ptr<ui::BoundingBox> create(int x, int y, int width, int height)
            {
                return std::make_shared<ui::BoundingBox>(x, y, width, height);
            }

            /// @brief Update override.
            void update(bool hasFocus) override;

            /// @brief Render override.
            void render(sdl::SharedTexture &target, bool hasFocus) override;

            /// @brief Sets the X and Y coord.
            /// @param x New X coord.
            /// @param y New Y coord.
            void set_xy(int x, int y);

            /// @brief Sets the width and height of the bounding box.
            /// @param width New width.
            /// @param height New height.
            void set_width_height(int width, int height);

            /// @brief Passing this to the set functions will keep the same coord.
            static inline constexpr int NO_SET = -1;

        private:
            /// @brief X coord to render to.
            int m_x{};

            /// @brief Y coord to rend to.
            int m_y{};

            /// @brief Width of the box.
            int m_width{};

            /// @brief Height of the box.
            int m_height{};

            /// @brief Color for the bounding box.
            ui::ColorMod m_colorMod{};

            /// @brief This is shared by all instances.
            static inline sdl::SharedTexture sm_corners{};

            /// @brief Loads ^
            void initialize_static_members();
    };
}

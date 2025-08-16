#pragma once
#include "sdl.hpp"
#include "ui/Element.hpp"

#include <memory>
#include <vector>

namespace ui
{
    class SlideOutPanel final : public ui::Element
    {
        public:
            /// @brief Enum for which side of the screen the panel slides from.
            enum class Side
            {
                Left,
                Right
            };

            /// @brief Constructor.
            /// @param width Width of the panel in pixels.
            /// @param side Which side of the screen the panel slides out from.
            SlideOutPanel(int width, SlideOutPanel::Side side);

            /// @brief Required destructor.
            ~SlideOutPanel() {};

            /// @brief Creates and returns and new ui::SlideOutPanel instance.
            static inline std::shared_ptr<ui::SlideOutPanel> create(int width, SlideOutPanel::Side side)
            {
                return std::make_shared<ui::SlideOutPanel>(width, side);
            }

            /// @brief Runs the update routine.
            /// @param hasFocus Whether or not the calling state has focus.
            void update(bool hasFocus) override;

            /// @brief Runs the render routine.
            /// @param target Target to render to.
            /// @param hasFocus Whether or the the calling state has focus.
            void render(sdl::SharedTexture &target, bool hasFocus) override;

            /// @brief Clears the target to a semi-transparent black. To do: Maybe not hard coded color.
            void clear_target();

            /// @brief Resets the panel back to its default state.
            void reset();

            /// @brief Closes the panel.
            void close();

            /// @brief Returns if the panel is fully open.
            /// @return If the panel is fully open.
            bool is_open() const;

            /// @brief Returns if the panel is fully closed.
            /// @return If the panel is fully closed.
            bool is_closed();

            /// @brief Pushes a new element to the element vector.
            /// @param newElement New element to push.
            void push_new_element(std::shared_ptr<ui::Element> newElement);

            /// @brief Clears the element vector, freeing them in the process.
            void clear_elements();

            /// @brief Returns a pointer to the render target of the panel.
            /// @return Raw SDL_Texture pointer to target.
            sdl::SharedTexture &get_target();

        private:
            /// @brief Bool for whether panel is fully open or not.
            bool m_isOpen{};

            /// @brief Whether or not to close panel.
            bool m_closePanel{};

            /// @brief Current X coordinate to render to. Panels are always 720 pixels in height so no Y is required.
            double m_x{};

            /// @brief Width of the panel in pixels.
            int m_width{};

            /// @brief Target X position of panel.
            double m_targetX{};

            /// @brief Which side the panel is on.
            SlideOutPanel::Side m_side{};

            /// @brief Render target if panel.
            sdl::SharedTexture m_renderTarget{};

            /// @brief Vector of elements.
            std::vector<std::shared_ptr<ui::Element>> m_elements{};

            void slide_out_left();

            void slide_out_right();

            int get_absolute_x_distance();
    };
} // namespace ui

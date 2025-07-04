#pragma once
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/ColorMod.hpp"
#include "ui/Element.hpp"
#include "ui/TitleTile.hpp"
#include <vector>

namespace ui
{
    /// @brief Presents a grid of icons to select a title from.
    class TitleView : public ui::Element
    {
        public:
            /// @brief Creates a title view using passed user pointer.
            /// @param user User to use.
            TitleView(data::User *user);

            /// @brief Required destructor.
            ~TitleView() {};

            /// @brief Runs the update routine.
            /// @param hasFocus Whether the calling state has focus.
            void update(bool hasFocus);

            /// @brief Runs the render routine.
            /// @param target Target to render to.
            /// @param hasFocus Whether or not the calling state has focus.
            void render(SDL_Texture *target, bool hasFocus);

            /// @brief Returns index of the currently selected tile.
            /// @return Index of currently selected tile.
            int get_selected() const;

            /// @brief Forces a refresh of the view.
            void refresh();

            /// @brief Resets the view to its default, empty state.
            void reset();

        private:
            /// @brief Pointer to user passed.
            data::User *m_user = nullptr;
            /// @brief Y coordinate.
            double m_y = 28.0f;
            /// @brief Currently highlighted/selected title.
            int m_selected = 0;
            /// @brief X coordinate of the currently selected tile so it can be rendered over top of the rest.
            double m_selectedX;
            /// @brief Y coordinate. Same as above.
            double m_selectedY;
            /// @brief Color mod for bounding/selection pulse.
            ui::ColorMod m_colorMod;
            /// @brief Vector of selection tiles.
            std::vector<ui::TitleTile> m_titleTiles;
    };
} // namespace ui

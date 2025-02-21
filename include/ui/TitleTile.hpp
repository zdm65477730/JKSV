#pragma once
#include "sdl.hpp"

namespace ui
{
    /// @brief Tile for Titleview.
    class TitleTile
    {
        public:
            /// @brief Constructor.
            /// @param isFavorite Whether the title is a favorite and should have the little heart rendered.
            /// @param icon Shared texture pointer to the icon.
            TitleTile(bool isFavorite, sdl::SharedTexture icon);

            /// @brief Runs the update routine.
            /// @param isSelected Whether or not the tile is selected and needs to expand.
            void update(bool isSelected);

            /// @brief Runs the render routine.
            /// @param target Target to render to.
            /// @param x X coordinate to render to.
            /// @param y Y coordinate to render to.
            void render(SDL_Texture *target, int x, int y);

            /// @brief Resets the width and height of the tile.
            void reset(void);

            /// @brief Returns the render width in pixels.
            /// @return Render width.
            int get_width(void) const;

            /// @brief Returns the render height in pixels.
            /// @return Render height.
            int get_height(void) const;

        private:
            /// @brief Width in pixels to render icon at.
            int m_renderWidth = 128;
            /// @brief Height in pixels to render icon at.
            int m_renderHeight = 128;
            /// @brief Whether or not the title is a favorite.
            bool m_isFavorite = false;
            /// @brief Title's icon texture.
            sdl::SharedTexture m_icon = nullptr;
    };
} // namespace ui

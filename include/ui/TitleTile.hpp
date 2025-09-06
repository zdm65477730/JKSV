#pragma once
#include "sdl.hpp"

namespace ui
{
    /// @brief Tile for Titleview.
    class TitleTile final
    {
        public:
            /// @brief Constructor.
            /// @param isFavorite Whether the title is a favorite and should have the little heart rendered.
            /// @param icon Shared texture pointer to the icon.
            TitleTile(bool isFavorite, int index, sdl::SharedTexture icon);

            /// @brief Runs the update routine.
            /// @param isSelected Whether or not the tile is selected and needs to expand.
            void update(int selected);

            /// @brief Runs the render routine.
            /// @param target Target to render to.
            /// @param x X coordinate to render to.
            /// @param y Y coordinate to render to.
            void render(sdl::SharedTexture &target, int x, int y);

            /// @brief Resets the width and height of the tile.
            void reset() noexcept;

            /// @brief Returns the render width in pixels.
            /// @return Render width.
            int get_width() const noexcept;

            /// @brief Returns the render height in pixels.
            /// @return Render height.
            int get_height() const noexcept;

        private:
            /// @brief Width in pixels to render icon at.
            int m_renderWidth = 128;

            /// @brief Height in pixels to render icon at.
            int m_renderHeight = 128;

            /// @brief Whether or not the title is a favorite.
            bool m_isFavorite{};

            int m_index{};

            /// @brief Title's icon texture.
            sdl::SharedTexture m_icon{};
    };
} // namespace ui

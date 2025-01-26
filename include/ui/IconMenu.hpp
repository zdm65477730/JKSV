#pragma once
#include "ui/Menu.hpp"

namespace ui
{
    /// @brief This is a hacky derived class to use Menu's code with Icons instead.
    class IconMenu : public ui::Menu
    {
        public:
            /// @brief Default constructor.
            IconMenu(void) = default;

            /// @brief This constructor calls initialize.
            /// @param x X coordinate to render the menu to.
            /// @param y Y coordinate to render the menu to.
            /// @param renderTargetHeight Height of the render target to calculate how many options can be displayed at once.
            IconMenu(int x, int y, int renderTargetHeight);

            /// @brief Required destructor.
            ~IconMenu() {};

            /// @brief Initializes the menu.
            /// @param x X coordinate to render the menu to.
            /// @param y Y coordinate to render the menu to.
            /// @param renderTargetHeight Height of the render target to calculate how many options can be displayed at once.
            void initialize(int x, int y, int renderTargetHeight);

            /// @brief Runs the update routine.
            /// @param hasFocus Whether or not the containing state has focus.
            void update(bool hasFocus);

            /// @brief Runs the render routine.
            /// @param target Target to render to.
            /// @param hasFocus Whether or not the containing state has focus.
            void render(SDL_Texture *target, bool hasFocus);

            /// @brief Adds a new icon to the menu.
            /// @param newOption Icon to add.
            void addOption(sdl::SharedTexture newOption);

        private:
            /// @brief Vector of shared texture pointers to textures used.
            std::vector<sdl::SharedTexture> m_options;
    };
} // namespace ui

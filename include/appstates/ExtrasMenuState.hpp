#pragma once
#include "appstates/AppState.hpp"
#include "sdl.hpp"
#include "ui/Menu.hpp"

/// @brief Extras menu.
class ExtrasMenuState : public AppState
{
    public:
        /// @brief Constructor.
        ExtrasMenuState(void);

        /// @brief Required even if nothing happens.
        ~ExtrasMenuState() {};

        /// @brief Updates the menu.
        void update(void);

        /// @brief Renders the menu to screen.
        void render(void);

    private:
        /// @brief Menu
        ui::Menu m_extrasMenu;
        /// @brief Render target for menu.
        sdl::SharedTexture m_renderTarget;
};

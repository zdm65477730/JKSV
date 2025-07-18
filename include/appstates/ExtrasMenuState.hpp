#pragma once
#include "appstates/BaseState.hpp"
#include "sdl.hpp"
#include "ui/Menu.hpp"

/// @brief Extras menu.
class ExtrasMenuState final : public BaseState
{
    public:
        /// @brief Constructor.
        ExtrasMenuState();

        /// @brief Required even if nothing happens.
        ~ExtrasMenuState() {};

        /// @brief Updates the menu.
        void update() override;

        /// @brief Renders the menu to screen.
        void render() override;

    private:
        /// @brief Menu
        ui::Menu m_extrasMenu;

        /// @brief Render target for menu.
        sdl::SharedTexture m_renderTarget{};
};

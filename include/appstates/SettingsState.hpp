#pragma once
#include "appstates/AppState.hpp"
#include "sdl.hpp"
#include "ui/Menu.hpp"

/// @brief The state for settings.
class SettingsState final : public AppState
{
    public:
        /// @brief Constructs a new settings state.
        SettingsState();

        /// @brief Required destructor.
        ~SettingsState() {};

        /// @brief Runs the update routine.
        void update() override;

        /// @brief Runs the render routine.
        void render() override;

    private:
        /// @brief Menu for selecting and toggling settings.
        ui::Menu m_settingsMenu;

        /// @brief Render target to render to.
        sdl::SharedTexture m_renderTarget = nullptr;

        /// @brief X coordinate of the control guide in the bottom right corner.
        int m_controlGuideX = 0;

        /// @brief Runs a routine to update the menu strings for the menu.
        void update_menu_options();

        /// @brief Toggles or executes the code to changed the selected menu option.
        void toggle_options();
};

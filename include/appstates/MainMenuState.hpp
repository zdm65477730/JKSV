#pragma once
#include "appstates/AppState.hpp"
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/IconMenu.hpp"

/// @brief The main
class MainMenuState : public AppState
{
    public:
        /// @brief Creates and initializes the main menu.
        MainMenuState(void);

        /// @brief Required even if it does nothing.
        ~MainMenuState() {};

        /// @brief Runs update routine.
        void update(void) override;

        /// @brief Renders menu to screen.
        void render(void) override;

        /// @brief This function allows other states to signal to this one to refresh the views on next call to update();
        static void refresh_view_states(void);

    private:
        /// @brief Render target this state renders to.
        sdl::SharedTexture m_renderTarget = nullptr;

        /// @brief The background gradient.
        sdl::SharedTexture m_background = nullptr;

        /// @brief Icon for the settings option,
        sdl::SharedTexture m_settingsIcon = nullptr;

        /// @brief Icon for the extras option.
        sdl::SharedTexture m_extrasIcon = nullptr;

        /// @brief Special menu type that uses icons.
        ui::IconMenu m_mainMenu;

        /// @brief Pointer to control guide string so I don't need to call string::getByName every loop.
        const char *m_controlGuide = nullptr;

        /// @brief X coordinate of the control guide in the bottom right corner.
        int m_controlGuideX;

        /// @brief Vector of pointers to users.
        static inline std::vector<data::User *> sm_users;

        /// @brief Vector of views for each user, settings, and extras.
        static inline std::vector<std::shared_ptr<AppState>> sm_states;

        /// @brief For signaling refreshes are needed.
        static inline bool sm_refreshNeeded = false;
};

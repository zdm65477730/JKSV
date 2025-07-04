#pragma once
#include "appstates/AppState.hpp"
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/IconMenu.hpp"
#include <memory>

/// @brief The main
class MainMenuState final : public AppState
{
    public:
        /// @brief Creates and initializes the main menu.
        MainMenuState();

        /// @brief Required even if it does nothing.
        ~MainMenuState() {};

        /// @brief Runs update routine.
        void update() override;

        /// @brief Renders menu to screen.
        void render() override;

        /// @brief Signals to
        static void initialize_view_states();

        /// @brief Calls refresh on on view states in the vector.
        static void refresh_view_states();

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

        /// @brief This is the list of user pointers from data.
        static inline data::UserList sm_users;

        /// @brief This is the pointer to the settings state.
        static inline std::shared_ptr<AppState> sm_settingsState = nullptr;

        /// @brief This is the pointer to the extras state.
        static inline std::shared_ptr<AppState> sm_extrasState = nullptr;

        /// @brief This is the vector of title selection states.
        static inline std::vector<std::shared_ptr<AppState>> sm_states;
};

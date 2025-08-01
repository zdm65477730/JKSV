#pragma once
#include "appstates/BaseState.hpp"
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/IconMenu.hpp"

#include <memory>

/// @brief The main
class MainMenuState final : public BaseState
{
    public:
        /// @brief Creates and initializes the main menu.
        MainMenuState();

        /// @brief Required even if it does nothing.
        ~MainMenuState() {};

        /// @brief Returns a new MainMenuState
        static std::shared_ptr<MainMenuState> create();

        /// @brief Creates and returns a new MainMenuState. Pushes it automatically.
        static std::shared_ptr<MainMenuState> create_and_push();

        /// @brief Runs update routine.
        void update() override;

        /// @brief Renders menu to screen.
        void render() override;

        /// @brief Signals to
        static void initialize_view_states();

        /// @brief Calls refresh on on view states in the vector.
        static void refresh_view_states();

        // clang-format off
        struct DataStruct
        {
            data::UserList userList;
        };
        // clang-format on

        using TaskData = std::shared_ptr<MainMenuState::DataStruct>;

    private:
        /// @brief Render target this state renders to.
        sdl::SharedTexture m_renderTarget{};

        /// @brief The background gradient.
        sdl::SharedTexture m_background{};

        /// @brief Icon for the settings option,
        sdl::SharedTexture m_settingsIcon{};

        /// @brief Icon for the extras option.
        sdl::SharedTexture m_extrasIcon{};

        /// @brief Special menu type that uses icons.
        ui::IconMenu m_mainMenu;

        /// @brief Pointer to control guide string so I don't need to call string::getByName every loop.
        const char *m_controlGuide{};

        /// @brief X coordinate of the control guide in the bottom right corner.
        int m_controlGuideX{};

        /// @brief This is the data struct passed to tasks.
        MainMenuState::TaskData m_dataStruct{};

        /// @brief Records the size of the sm_users vector.
        static inline int sm_userCount{};

        /// @brief This is the list of user pointers from data.
        static inline data::UserList sm_users{};

        /// @brief This is the pointer to the settings state.
        static inline std::shared_ptr<BaseState> sm_settingsState{};

        /// @brief This is the pointer to the extras state.
        static inline std::shared_ptr<BaseState> sm_extrasState{};

        /// @brief This is the vector of title selection states.
        static inline std::vector<std::shared_ptr<BaseState>> sm_states{};

        /// @brief Creates the settings and extras.
        void initialize_settings_extras();

        /// @brief Pushes the icons to the main menu.
        void initialize_menu();

        /// @brief Initializes the data struct.
        void initialize_data_struct();

        /// @brief Pushes the target state to the vector.
        void push_target_state();

        /// @brief Creates the user option state.
        void create_user_options();

        /// @brief Backups up all save data for all users.
        void backup_all_for_all();
};

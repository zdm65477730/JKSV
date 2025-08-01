#pragma once
#include "appstates/BaseState.hpp"
#include "appstates/TitleSelectCommon.hpp"
#include "data/data.hpp"
#include "ui/Menu.hpp"
#include "ui/SlideOutPanel.hpp"

#include <memory>

/// @brief State that allows certain actions to be taken for users.
class UserOptionState final : public BaseState
{
    public:
        /// @brief Constructs a new UserOptionState.
        /// @param user Pointer to target user of the state.
        /// @param titleSelect Pointer to the selection state for refresh and rendering.
        UserOptionState(data::User *user, TitleSelectCommon *titleSelect);

        /// @brief Required destructor.
        ~UserOptionState() {};

        /// @brief Returns a new UserOptionState. See constructor.
        static std::shared_ptr<UserOptionState> create(data::User *user, TitleSelectCommon *titleSelect);

        /// @brief Creates, pushes, and returns a new UserOptionState.
        static std::shared_ptr<UserOptionState> create_and_push(data::User *user, TitleSelectCommon *titleSelect);

        /// @brief Runs the render routine.
        void update() override;

        /// @brief Runs the render routine.
        void render() override;

        /// @brief Signals to the main update() function that a refresh is needed.
        /// @note Like this to prevent threading headaches.
        void data_and_view_refresh_required();

        // clang-format off
        struct DataStruct
        {
            data::User *user{};
            UserOptionState *spawningState{};
        };
        // clang-format on

    private:
        /// @brief Pointer to the target user.
        data::User *m_user{};

        /// @brief Pointer to the selection view.
        TitleSelectCommon *m_titleSelect{};

        /// @brief Menu that displays the options available.
        ui::Menu m_userOptionMenu;

        /// @brief Shared pointer to pass data to tasks and functions.
        std::shared_ptr<UserOptionState::DataStruct> m_dataStruct{};

        /// @brief This allows spawned tasks to signal to the main thread to update the view.
        bool m_refreshRequired{};

        /// @brief Slide panel all instances shared.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_menuPanel{};

        /// @brief Creates the panel if it hasn't been yet.
        void create_menu_panel();

        /// @brief Creates and pushes the strings needed for the menu
        void load_menu_strings();

        /// @brief Assigns the data within the struct to point where it needs to.
        void initialize_data_struct();

        /// @brief Starts the backup all operation.
        void backup_all();

        /// @brief Creates and pushes a new save creation menu.
        void create_save_create();

        /// @brief Launches the create all save data for use task.
        void create_all_save_data();

        /// @brief Deletes all save data for the user.
        void delete_all_save_data();
};

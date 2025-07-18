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

        /// @brief Runs the render routine.
        void update() override;

        /// @brief Runs the render routine.
        void render() override;

        /// @brief Signals to the main update() function that a refresh is needed.
        /// @note Like this to prevent threading headaches.
        void data_and_view_refresh_required();

        /// @brief Struct used for passing data to functions/tasks.
        typedef struct
        {
                /// @brief Pointer to the target user.
                data::User *m_user{};

                /// @brief Pointer to >this spawning state.
                UserOptionState *m_spawningState{};
        } DataStruct;

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
        static inline std::unique_ptr<ui::SlideOutPanel> m_menuPanel{};
};

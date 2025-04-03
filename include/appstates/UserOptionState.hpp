#pragma once
#include "appstates/AppState.hpp"
#include "appstates/TitleSelectCommon.hpp"
#include "data/data.hpp"
#include "ui/Menu.hpp"
#include "ui/SlideOutPanel.hpp"
#include <memory>

/// @brief State that allows certain actions to be taken for users.
class UserOptionState : public AppState
{
    public:
        /// @brief Constructs a new UserOptionState.
        /// @param user Pointer to target user of the state.
        /// @param titleSelect Pointer to the selection state for refresh and rendering.
        UserOptionState(data::User *user, TitleSelectCommon *titleSelect);

        /// @brief Required destructor.
        ~UserOptionState() {};

        /// @brief Runs the render routine.
        void update(void) override;

        /// @brief Runs the render routine.
        void render(void) override;

    private:
        /// @brief Pointer to the target user.
        data::User *m_user;

        /// @brief Pointer to the selection view.
        TitleSelectCommon *m_titleSelect;

        /// @brief Menu that displays the options available.
        ui::Menu m_userOptionMenu;

        /// @brief Slide panel all instances shared.
        static inline std::unique_ptr<ui::SlideOutPanel> m_menuPanel = nullptr;
};

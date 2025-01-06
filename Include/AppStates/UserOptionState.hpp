#pragma once
#include "AppStates/AppState.hpp"
#include "Data/User.hpp"
#include "TitleSelectCommon.hpp"
#include "UI/Menu.hpp"
#include "UI/SlideOutPanel.hpp"
#include <memory>

class UserOptionState : public AppState
{
    public:
        // Takes a pointer to the target user and the view so it can render and refresh it.
        UserOptionState(Data::User *User, TitleSelectCommon *TitleSelect);
        ~UserOptionState() {};

        void Update(void);
        void Render(void);

    private:
        // Target user
        Data::User *m_User;
        // Title view to render and refresh
        TitleSelectCommon *m_TitleSelect;
        // Menu
        UI::Menu m_UserOptionMenu;
        // Static panel shared by all instances.
        static inline std::unique_ptr<UI::SlideOutPanel> m_MenuPanel = nullptr;
};

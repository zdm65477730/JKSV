#pragma once
#include "AppStates/AppState.hpp"
#include "Data/User.hpp"
#include "UI/Menu.hpp"
#include "UI/SlideOutPanel.hpp"
#include <memory>

class UserOptionState : public AppState
{
    public:
        UserOptionState(Data::User *User);
        ~UserOptionState() {};

        void Update(void);
        void Render(void);

    private:
        // Target user
        Data::User *m_User;
        // Menu
        UI::Menu m_UserOptionMenu;
        // Static panel shared by all instances.
        static inline std::unique_ptr<UI::SlideOutPanel> m_MenuPanel = nullptr;
};

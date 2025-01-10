#pragma once
#include "AppStates/AppState.hpp"
#include "AppStates/TitleSelectCommon.hpp"
#include "Data/Data.hpp"
#include "UI/Menu.hpp"
#include "UI/SlideOutPanel.hpp"
#include <memory>

class SaveCreateState : public AppState
{
    public:
        // Takes pointer to target user & their title selection for rendering and refreshing.
        SaveCreateState(Data::User *TargetUser, TitleSelectCommon *TitleSelect);
        ~SaveCreateState() {};

        void Update(void);
        void Render(void);

    private:
        // Pointer to user and title select
        Data::User *m_User;
        TitleSelectCommon *m_TitleSelect;
        // Menu
        UI::Menu m_SaveMenu;
        // Vector of pointers to save info we're using
        std::vector<Data::TitleInfo *> m_TitleInfoVector;
        // All instances shared this so they're static.
        static inline std::unique_ptr<UI::SlideOutPanel> m_SlidePanel = nullptr;
};

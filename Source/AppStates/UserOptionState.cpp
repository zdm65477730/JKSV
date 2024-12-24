#include "AppStates/UserOptionState.hpp"
#include "Input.hpp"
#include "StringUtil.hpp"
#include "Strings.hpp"
#include "System/ProgressTask.hpp"

UserOptionState::UserOptionState(Data::User *User) : m_User(User), m_UserOptionMenu(8, 8, 460, 22, 720)
{
    // Check if panel needs to be created. It's shared by all instances.
    if (!m_MenuPanel)
    {
        m_MenuPanel = std::make_unique<UI::SlideOutPanel>(480, UI::SlideOutPanel::Side::Right);
    }

    const char *CurrentString = nullptr;
    int CurrentStringIndex = 0;
    while ((CurrentString = Strings::GetByName(Strings::Names::UserOptions, CurrentStringIndex++)) != nullptr)
    {
        m_UserOptionMenu.AddOption(StringUtil::GetFormattedString(CurrentString, m_User->GetNickname()));
    }
}

void UserOptionState::Update(void)
{
    m_MenuPanel->Update(AppState::HasFocus());
    if (Input::ButtonPressed(HidNpadButton_B))
    {
        m_MenuPanel->Close();
    }
    else if (m_MenuPanel->IsClosed())
    {
        AppState::Deactivate();
        m_MenuPanel->Reset();
    }

    m_UserOptionMenu.Update(AppState::HasFocus());
}

void UserOptionState::Render(void)
{
    m_MenuPanel->ClearTarget();
    m_UserOptionMenu.Render(m_MenuPanel->Get(), AppState::HasFocus());
    m_MenuPanel->Render(NULL, AppState::HasFocus());
}

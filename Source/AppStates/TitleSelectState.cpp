#include "AppStates/TitleSelectState.hpp"
#include "AppStates/BackupMenuState.hpp"
#include "AppStates/MainMenuState.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "FS/SaveMount.hpp"
#include "FsLib.hpp"
#include "Input.hpp"
#include "JKSV.hpp"
#include "SDL.hpp"
#include "Strings.hpp"
#include <string_view>

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

TitleSelectState::TitleSelectState(Data::User *User)
    : TitleSelectCommon(), m_User(User),
      m_RenderTarget(SDL::TextureManager::CreateLoadTexture(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_TitleView(m_User) {};

void TitleSelectState::Update(void)
{
    m_TitleView.Update(AppState::HasFocus());

    if (Input::ButtonPressed(HidNpadButton_A))
    {
        // Get data needed to mount save.
        uint64_t ApplicationID = m_User->GetApplicationIDAt(m_TitleView.GetSelected());
        Data::TitleInfo *TitleInfo = Data::GetTitleInfoByID(ApplicationID);

        // Path to output to.
        FsLib::Path TargetPath = Config::GetWorkingDirectory() / TitleInfo->GetPathSafeTitle();

        if ((FsLib::DirectoryExists(TargetPath) || FsLib::CreateDirectory(TargetPath)) &&
            FS::MountSaveData(*m_User->GetSaveInfoByID(ApplicationID), FS::DEFAULT_SAVE_MOUNT))
        {
            JKSV::PushState(std::make_shared<BackupMenuState>(m_User, TitleInfo));
        }
    }
    else if (Input::ButtonPressed(HidNpadButton_B))
    {
        // This will reset all the tiles so they're 128x128.
        m_TitleView.Reset();
        AppState::Deactivate();
    }
    else if (Input::ButtonPressed(HidNpadButton_Y))
    {
        Config::AddRemoveFavorite(m_User->GetApplicationIDAt(m_TitleView.GetSelected()));
        // MainMenuState has all the Users and views, so have it refresh.
        MainMenuState::RefreshViewStates();
    }
}

void TitleSelectState::Render(void)
{
    m_RenderTarget->Clear(Colors::Transparent);
    m_TitleView.Render(m_RenderTarget->Get(), AppState::HasFocus());
    TitleSelectCommon::RenderControlGuide();
    m_RenderTarget->Render(NULL, 201, 91);
}

void TitleSelectState::Refresh(void)
{
    m_TitleView.Refresh();
}

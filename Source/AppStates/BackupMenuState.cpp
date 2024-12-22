#include "AppStates/BackupMenuState.hpp"
#include "AppStates/ProgressState.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "FS/FileIO.hpp"
#include "FS/SaveMount.hpp"
#include "FS/ZipIO.hpp"
#include "FsLib.hpp"
#include "Input.hpp"
#include "JKSV.hpp"
#include "Keyboard.hpp"
#include "SDL.hpp"
#include "Strings.hpp"
#include "System/ProgressTask.hpp"
#include <cstring>

// This is the function to create new backups.
static void CreateNewBackup(System::ProgressTask *Task, FsLib::Path DestinationPath)
{
    // This extension search is lazy and needs to be revised.
    if (Config::GetByKey(Config::Keys::ExportToZip) || std::strstr(DestinationPath.CString(), ".zip") != NULL)
    {
        zipFile NewBackup = zipOpen64(DestinationPath.CString(), APPEND_STATUS_CREATE);
        FS::CopyDirectoryToZip(FS::DEFAULT_SAVE_PATH, NewBackup, Task);
        zipClose(NewBackup, NULL);
    }
    else
    {
        FS::CopyDirectory(FS::DEFAULT_SAVE_PATH, DestinationPath, 0, {}, Task);
    }
    Task->Finished();
}

BackupMenuState::BackupMenuState(Data::User *User, Data::TitleInfo *TitleInfo)
    : m_User(User), m_TitleInfo(TitleInfo), m_DirectoryPath(Config::GetWorkingDirectory() / m_TitleInfo->GetPathSafeTitle()),
      m_TitleWidth(SDL::Text::GetWidth(22, m_TitleInfo->GetTitle())), m_TitleTimer(3000)
{
    if (!m_Initialized)
    {
        m_PanelWidth = SDL::Text::GetWidth(22, Strings::GetByName(Strings::Names::ControlGuides, 2)) + 64;
        // To do: Give classes an alternate so they don't have to be constructed.
        m_BackupMenu = std::make_unique<UI::Menu>(8, 8, m_PanelWidth - 24, 24, 600);
        m_SlidePanel = std::make_unique<UI::SlideOutPanel>(m_PanelWidth, UI::SlideOutPanel::Side::Right);
        m_MenuTarget =
            SDL::TextureManager::CreateLoadTexture("BackupMenuTarget", m_PanelWidth, 600, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
        m_Initialized = true;
    }

    if (m_TitleWidth >= m_PanelWidth)
    {
        m_ScrollTitle = true;
        m_TitleX = 8;
    }
    else
    {
        m_TitleX = (m_PanelWidth / 2) - (m_TitleWidth / 2);
    }
    BackupMenuState::RefreshListing();
}

void BackupMenuState::Update(void)
{
    if (Input::ButtonPressed(HidNpadButton_A) && m_BackupMenu->GetSelected() == 0)
    {
        // Get name for backup.
        char BackupName[0x64] = {0};
        if (!Keyboard::GetInput(SwkbdType_QWERTY, "", Strings::GetByName(Strings::Names::KeyboardStrings, 0), BackupName, 0x64))
        {
            return;
        }
        // To do: This isn't a good way to check for this... Check to make sure zip has zip extension.
        if (Config::GetByKey(Config::Keys::ExportToZip) && std::strstr(BackupName, ".zip") == NULL)
        {
            // To do: I should check this.
            std::strcat(BackupName, ".zip");
        }
        else if (!Config::GetByKey(Config::Keys::ExportToZip) && !std::strstr(BackupName, ".zip") &&
                 !FsLib::DirectoryExists(m_DirectoryPath / BackupName) && !FsLib::CreateDirectory(m_DirectoryPath / BackupName))
        {
            return;
        }
        // Push the task.
        JKSV::PushState(std::make_shared<ProgressState>(CreateNewBackup, m_DirectoryPath / BackupName));
    }
    else if (Input::ButtonPressed(HidNpadButton_B))
    {
        FsLib::CloseFileSystem(FS::DEFAULT_SAVE_MOUNT);
        m_SlidePanel->Close();
    }
    else if (m_SlidePanel->IsClosed())
    {
        m_SlidePanel->Reset();
        AppState::Deactivate();
    }

    m_SlidePanel->Update(AppState::HasFocus());
    // This state bypasses the Slideout panel's normal behavior because it kind of has to.
    m_BackupMenu->Update(AppState::HasFocus());
}

void BackupMenuState::Render(void)
{
    // Clear panel target.
    m_SlidePanel->ClearTarget();
    // Render the current title's name.
    BackupMenuState::RenderTitle();
    SDL::RenderLine(m_SlidePanel->Get(), 10, 42, m_PanelWidth - 20, 42, Colors::White);
    SDL::RenderLine(m_SlidePanel->Get(), 10, 648, m_PanelWidth - 20, 648, Colors::White);
    SDL::Text::Render(m_SlidePanel->Get(),
                      32,
                      673,
                      22,
                      SDL::Text::NO_TEXT_WRAP,
                      Colors::White,
                      Strings::GetByName(Strings::Names::ControlGuides, 2));

    // Clear menu target.
    m_MenuTarget->Clear(Colors::Transparent);
    // Render menu to it.
    m_BackupMenu->Render(m_MenuTarget->Get(), AppState::HasFocus());
    // Render it to panel target.
    m_MenuTarget->Render(m_SlidePanel->Get(), 0, 43);
    m_SlidePanel->Render(NULL, AppState::HasFocus());
}

void BackupMenuState::RefreshListing(void)
{
    m_DirectoryListing.Open(m_DirectoryPath);
    if (!m_DirectoryListing.IsOpen())
    {
        return;
    }

    m_BackupMenu->Reset();
    m_BackupMenu->AddOption(Strings::GetByName(Strings::Names::BackupMenu, 0));
    for (int64_t i = 0; i < m_DirectoryListing.GetEntryCount(); i++)
    {
        m_BackupMenu->AddOption(m_DirectoryListing[i]);
    }
}

void BackupMenuState::RenderTitle(void)
{
    SDL_Texture *SlidePanelTarget = m_SlidePanel->Get();

    if (m_ScrollTitle && m_ScrollTriggered && m_TitleX > -(m_TitleWidth + 8))
    {
        m_TitleX -= 2;
    }
    else if (m_ScrollTitle && m_ScrollTriggered && m_TitleX <= -(m_TitleWidth + 8))
    {
        m_TitleX = 8;
        m_ScrollTriggered = false;
        m_TitleTimer.Restart();
    }
    else if (m_ScrollTitle && m_TitleTimer.IsTriggered())
    {
        m_ScrollTriggered = true;
    }

    if (m_ScrollTitle && m_ScrollTriggered)
    {
        // This is just a trick, or maybe the only way to accomplish this. Either way, it works.
        // Render title first time.
        SDL::Text::Render(SlidePanelTarget, m_TitleX, 8, 22, SDL::Text::NO_TEXT_WRAP, Colors::White, m_TitleInfo->GetTitle());
        // Render it again following the first.
        SDL::Text::Render(SlidePanelTarget,
                          m_TitleX + m_TitleWidth + 16,
                          8,
                          22,
                          SDL::Text::NO_TEXT_WRAP,
                          Colors::White,
                          m_TitleInfo->GetTitle());
    }
    else
    {
        SDL::Text::Render(SlidePanelTarget, m_TitleX, 8, 22, SDL::Text::NO_TEXT_WRAP, Colors::White, m_TitleInfo->GetTitle());
    }
}

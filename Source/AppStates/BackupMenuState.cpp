#include "AppStates/BackupMenuState.hpp"
#include "AppStates/ConfirmState.hpp"
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
#include "Logger.hpp"
#include "SDL.hpp"
#include "StringUtil.hpp"
#include "Strings.hpp"
#include "System/ProgressTask.hpp"
#include "System/Task.hpp"
#include <cstring>

// This struct is used to pass data to Restore, Delete, and upload.
struct TargetStruct : ConfirmStruct
{
        FsLib::Path TargetPath;
        uint64_t JournalSize = 0;
        BackupMenuState *CreatingState = nullptr;
};

// This is the function to create new backups.
static void CreateNewBackup(System::ProgressTask *Task, FsLib::Path DestinationPath, BackupMenuState *CreatingState)
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
    CreatingState->RefreshListing();
    Task->Finished();
}

static void RestoreBackup(System::ProgressTask *Task, std::shared_ptr<ConfirmStruct> DataStruct)
{
    // Wipe the save root first.
    if (!FsLib::DeleteDirectoryRecursively(FS::DEFAULT_SAVE_PATH))
    {
        Logger::Log("Error restoring save. Unable to reset save data: %s", FsLib::GetErrorString());
        Task->Finished();
        return;
    }

    // Cast struct to what we really need.
    std::shared_ptr<TargetStruct> Data = std::static_pointer_cast<TargetStruct>(DataStruct);

    if (FsLib::DirectoryExists(Data->TargetPath))
    {
        FS::CopyDirectory(Data->TargetPath, FS::DEFAULT_SAVE_PATH, Data->JournalSize, FS::DEFAULT_SAVE_MOUNT, Task);
    }
    else if (std::strstr(Data->TargetPath.CString(), ".zip") != NULL)
    {
        unzFile TargetZip = unzOpen64(Data->TargetPath.CString());
        if (!TargetZip)
        {
            Logger::Log("Error opening zip for reading.");
            Task->Finished();
            return;
        }
        FS::CopyZipToDirectory(TargetZip, FS::DEFAULT_SAVE_PATH, Data->JournalSize, FS::DEFAULT_SAVE_MOUNT, Task);
        unzClose(TargetZip);
    }
    else
    {
        FS::CopyFile(Data->TargetPath, FS::DEFAULT_SAVE_PATH, Data->JournalSize, FS::DEFAULT_SAVE_MOUNT, Task);
    }
    Task->Finished();
}

static void DeleteBackup(System::Task *Task, std::shared_ptr<ConfirmStruct> DataStruct)
{
    std::shared_ptr<TargetStruct> Data = std::static_pointer_cast<TargetStruct>(DataStruct);

    if (Task)
    {
        Task->SetStatus(Strings::GetByName(Strings::Names::DeletingFiles, 0), Data->TargetPath.CString());
    }

    if (FsLib::DirectoryExists(Data->TargetPath) && !FsLib::DeleteDirectoryRecursively(Data->TargetPath))
    {
        Logger::Log("Error deleting folder backup: %s", FsLib::GetErrorString());
    }
    else if (!FsLib::DeleteFile(Data->TargetPath))
    {
        Logger::Log("Error deleting backup: %s", FsLib::GetErrorString());
    }
    Data->CreatingState->RefreshListing();
    Task->Finished();
}

BackupMenuState::BackupMenuState(Data::User *User, Data::TitleInfo *TitleInfo, FsSaveDataType SaveType)
    : m_User(User), m_TitleInfo(TitleInfo), m_SaveType(SaveType),
      m_DirectoryPath(Config::GetWorkingDirectory() / m_TitleInfo->GetPathSafeTitle()),
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
        char BackupName[0x81] = {0};

        // Set backup to default.
        std::snprintf(BackupName, 0x64, "%s - %s", m_User->GetPathSafeNickname(), StringUtil::GetDateString().c_str());

        if (!Input::ButtonHeld(HidNpadButton_ZR) &&
            !Keyboard::GetInput(SwkbdType_QWERTY, BackupName, Strings::GetByName(Strings::Names::KeyboardStrings, 0), BackupName, 0x80))
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
        JKSV::PushState(std::make_shared<ProgressState>(CreateNewBackup, m_DirectoryPath / BackupName, this));
    }
    else if (Input::ButtonPressed(HidNpadButton_Y) && m_BackupMenu->GetSelected() > 0 &&
             (m_SaveType != FsSaveDataType_System || Config::GetByKey(Config::Keys::AllowSystemSaveWriting)))
    {
        int Selected = m_BackupMenu->GetSelected() - 1;

        std::shared_ptr<ConfirmStruct> DataStruct = std::make_shared<TargetStruct>();
        std::static_pointer_cast<TargetStruct>(DataStruct)->TargetPath = m_DirectoryPath / m_DirectoryListing[Selected];
        std::static_pointer_cast<TargetStruct>(DataStruct)->JournalSize = m_TitleInfo->GetJournalSize(m_SaveType);

        std::string QueryString =
            StringUtil::GetFormattedString(Strings::GetByName(Strings::Names::BackupMenuConfirmations, 0), m_DirectoryListing[Selected]);

        JKSV::PushState(std::make_shared<ConfirmState<System::ProgressTask, ProgressState>>(QueryString, false, RestoreBackup, DataStruct));
    }
    else if (Input::ButtonPressed(HidNpadButton_X) && m_BackupMenu->GetSelected() > 0)
    {
        // Selected needs to be offset by one to account for New
        int Selected = m_BackupMenu->GetSelected() - 1;

        // Create struct to pass.
        std::shared_ptr<ConfirmStruct> DataStruct = std::make_shared<TargetStruct>();
        std::static_pointer_cast<TargetStruct>(DataStruct)->TargetPath = m_DirectoryPath / m_DirectoryListing[Selected];
        std::static_pointer_cast<TargetStruct>(DataStruct)->CreatingState = this;

        // Get the string.
        std::string QueryString =
            StringUtil::GetFormattedString(Strings::GetByName(Strings::Names::BackupMenuConfirmations, 1), m_DirectoryListing[Selected]);

        // Create/push new state.
        JKSV::PushState(std::make_shared<ConfirmState<System::Task, TaskState>>(QueryString, false, DeleteBackup, DataStruct));
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

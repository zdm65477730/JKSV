#include "AppStates/UserOptionState.hpp"
#include "AppStates/ProgressState.hpp"
#include "AppStates/SaveCreateState.hpp"
#include "Config.hpp"
#include "Data/Data.hpp"
#include "FS/FileIO.hpp"
#include "FS/SaveMount.hpp"
#include "FS/ZipIO.hpp"
#include "FsLib.hpp"
#include "Input.hpp"
#include "JKSV.hpp"
#include "Logger.hpp"
#include "StringUtil.hpp"
#include "Strings.hpp"
#include "System/ProgressTask.hpp"

static void BackupAllForUser(System::ProgressTask *Task, Data::User *TargetUser)
{
    for (size_t i = 0; i < TargetUser->GetTotalDataEntries(); i++)
    {
        // This should be safe like this....
        FsSaveDataInfo *CurrentSaveInfo = TargetUser->GetSaveInfoAt(i);
        Data::TitleInfo *CurrentTitle = Data::GetTitleInfoByID(CurrentSaveInfo->application_id);

        if (!CurrentSaveInfo || !CurrentTitle)
        {
            Logger::Log("One of these is nullptr?");
            continue;
        }

        // Try to create target game folder.
        FsLib::Path GameFolder = Config::GetWorkingDirectory() / CurrentTitle->GetPathSafeTitle();
        if (!FsLib::DirectoryExists(GameFolder) && !FsLib::CreateDirectory(GameFolder))
        {
            Logger::Log("Error creating target game folder: %s", FsLib::GetErrorString());
            continue;
        }

        // Try to mount save data.
        bool SaveMounted = FS::MountSaveData(*CurrentSaveInfo, FS::DEFAULT_SAVE_MOUNT);

        if (CurrentTitle && SaveMounted && Config::GetByKey(Config::Keys::ExportToZip))
        {
            FsLib::Path TargetPath = Config::GetWorkingDirectory() / CurrentTitle->GetPathSafeTitle() / TargetUser->GetPathSafeNickname() +
                                     " - " + StringUtil::GetDateString() + ".zip";

            zipFile TargetZip = zipOpen64(TargetPath.CString(), APPEND_STATUS_CREATE);
            if (!TargetZip)
            {
                Logger::Log("Error creating zip: %s", FsLib::GetErrorString());
                continue;
            }
            FS::CopyDirectoryToZip(FS::DEFAULT_SAVE_PATH, TargetZip, Task);
            zipClose(TargetZip, NULL);
        }
        else if (CurrentTitle && SaveMounted)
        {
            FsLib::Path TargetPath = Config::GetWorkingDirectory() / CurrentTitle->GetPathSafeTitle() / TargetUser->GetPathSafeNickname() +
                                     " - " + StringUtil::GetDateString();

            if (!FsLib::CreateDirectory(TargetPath))
            {
                Logger::Log("Error creating backup directory: %s", FsLib::GetErrorString());
                continue;
            }
            FS::CopyDirectory(FS::DEFAULT_SAVE_PATH, TargetPath, 0, {}, Task);
        }

        if (SaveMounted)
        {
            FsLib::CloseFileSystem(FS::DEFAULT_SAVE_MOUNT);
        }
    }
    Task->Finished();
}

UserOptionState::UserOptionState(Data::User *User, TitleSelectCommon *TitleSelect)
    : m_User(User), m_TitleSelect(TitleSelect), m_UserOptionMenu(8, 8, 460, 22, 720)
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

    if (Input::ButtonPressed(HidNpadButton_A))
    {
        switch (m_UserOptionMenu.GetSelected())
        {
            case 0:
            {
                JKSV::PushState(std::make_shared<ProgressState>(BackupAllForUser, m_User));
            }
            break;

            case 1:
            {
                JKSV::PushState(std::make_shared<SaveCreateState>(m_User, m_TitleSelect));
            }
            break;
        }
    }
    else if (Input::ButtonPressed(HidNpadButton_B))
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
    // Render target user's title selection screen.
    m_TitleSelect->Render();

    // Render panel.
    m_MenuPanel->ClearTarget();
    m_UserOptionMenu.Render(m_MenuPanel->Get(), AppState::HasFocus());
    m_MenuPanel->Render(NULL, AppState::HasFocus());
}

#include "AppStates/SaveCreateState.hpp"
#include "AppStates/TaskState.hpp"
#include "Data/Data.hpp"
#include "Input.hpp"
#include "JKSV.hpp"
#include "Logger.hpp"
#include "Strings.hpp"
#include "System/Task.hpp"
#include <algorithm>
#include <string>
#include <switch.h>
#include <vector>

// This sorts the vector alphabetically so stuff is easier to find
static bool CompareInfo(Data::TitleInfo *InfoA, Data::TitleInfo *InfoB)
{
    const char *TitleA = InfoA->GetTitle();
    const char *TitleB = InfoB->GetTitle();

    size_t TitleALength = std::char_traits<char>::length(TitleA);
    size_t TitleBLength = std::char_traits<char>::length(TitleB);
    size_t ShortestTitle = TitleALength < TitleBLength ? TitleALength : TitleBLength;
    // To do: This doesn't take into account which is the shortest title. This can still go out-of-bounds.
    for (size_t i = 0, j = 0; i < ShortestTitle;)
    {
        uint32_t CodepointA = 0;
        uint32_t CodepointB = 0;

        ssize_t UnitACount = decode_utf8(&CodepointA, reinterpret_cast<const uint8_t *>(&TitleA[i]));
        ssize_t UnitBCount = decode_utf8(&CodepointB, reinterpret_cast<const uint8_t *>(&TitleB[j]));

        if (UnitACount <= 0 || UnitBCount <= 0)
        {
            return false;
        }

        if (CodepointA != CodepointB)
        {
            return CodepointA < CodepointB;
        }

        i += UnitACount;
        j += UnitBCount;
    }
    return false;
}

// This attempts to create the save data for the given user. It will fail if it already exists.
static void CreateSaveDataFor(System::Task *Task, Data::User *TargetUser, Data::TitleInfo *TitleInfo)
{
    // Attributes of save data. To do: Owner ID might not be the same. Need to research.
    FsSaveDataAttribute SaveAttributes = {.application_id = TitleInfo->GetSaveDataOwnerID(),
                                          .uid = TargetUser->GetAccountID(),
                                          .system_save_data_id = 0,
                                          .save_data_type = FsSaveDataType_Account,
                                          .save_data_rank = FsSaveDataRank_Primary,
                                          .save_data_index = 0};

    // Creation info for save data.
    FsSaveDataCreationInfo SaveCreationInfo = {.save_data_size = static_cast<int64_t>(TitleInfo->GetSaveDataSize(FsSaveDataType_Account)),
                                               .journal_size = static_cast<int64_t>(TitleInfo->GetJournalSize(FsSaveDataType_Account)),
                                               .available_size = 0x4000,
                                               .owner_id = TitleInfo->GetSaveDataOwnerID(),
                                               .flags = 0,
                                               .save_data_space_id = FsSaveDataSpaceId_User};

    // Save meta
    FsSaveDataMetaInfo SaveMetaInfo = {.size = 0x40060, .type = FsSaveDataMetaType_Thumbnail};

    // Set task status
    Task->SetStatus(Strings::GetByName(Strings::Names::CreatingSaveDataFor, 0), TitleInfo->GetTitle());

    Result FsError = fsCreateSaveDataFileSystem(&SaveAttributes, &SaveCreationInfo, &SaveMetaInfo);
    if (R_FAILED(FsError))
    {
        Logger::Log("Error creating save data for %016llX: 0x%X.", TitleInfo->GetSaveDataOwnerID(), FsError);
    }

    Task->Finished();
}

SaveCreateState::SaveCreateState(Data::User *TargetUser, TitleSelectCommon *TitleSelect)
    : m_User(TargetUser), m_TitleSelect(TitleSelect), m_SaveMenu(8, 8, 624, 22, 720)
{
    // If the panel is null, create it.
    if (!m_SlidePanel)
    {
        // Create panel and menu.
        m_SlidePanel = std::make_unique<UI::SlideOutPanel>(640, UI::SlideOutPanel::Side::Right);
    }

    // Get title info vector and copy titles to menu.
    Data::GetTitleInfoByType(FsSaveDataType_Account, m_TitleInfoVector);

    // Sort it by alpha
    std::sort(m_TitleInfoVector.begin(), m_TitleInfoVector.end(), CompareInfo);

    for (size_t i = 0; i < m_TitleInfoVector.size(); i++)
    {
        m_SaveMenu.AddOption(m_TitleInfoVector.at(i)->GetTitle());
    }
}

void SaveCreateState::Update(void)
{
    m_SlidePanel->Update(AppState::HasFocus());
    m_SaveMenu.Update(AppState::HasFocus());

    if (Input::ButtonPressed(HidNpadButton_A))
    {
        Data::TitleInfo *TargetTitle = m_TitleInfoVector.at(m_SaveMenu.GetSelected());
        JKSV::PushState(std::make_shared<TaskState>(CreateSaveDataFor, m_User, TargetTitle));
    }
    else if (Input::ButtonPressed(HidNpadButton_B))
    {
        m_SlidePanel->Close();
    }
    else if (m_SlidePanel->IsClosed())
    {
        m_SlidePanel->Reset();
        AppState::Deactivate();
    }
}

void SaveCreateState::Render(void)
{
    // Clear slide target, render menu, render slide to frame buffer.
    m_SlidePanel->ClearTarget();
    m_SaveMenu.Render(m_SlidePanel->Get(), AppState::HasFocus());
    m_SlidePanel->Render(NULL, AppState::HasFocus());
}

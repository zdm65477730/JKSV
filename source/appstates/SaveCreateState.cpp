#include "appstates/SaveCreateState.hpp"
#include "JKSV.hpp"
#include "appstates/TaskState.hpp"
#include "data/data.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include "system/Task.hpp"
#include <algorithm>
#include <string>
#include <switch.h>
#include <vector>

// This sorts the vector alphabetically so stuff is easier to find
static bool compareInfo(data::TitleInfo *infoA, data::TitleInfo *infoB)
{
    const char *titleA = infoA->getTitle();
    const char *titleB = infoB->getTitle();

    size_t titleALength = std::char_traits<char>::length(titleA);
    size_t titleBLength = std::char_traits<char>::length(titleB);
    size_t shortestTitle = titleALength < titleBLength ? titleALength : titleBLength;
    // To do: This doesn't take into account which is the shortest title. This can still go out-of-bounds.
    for (size_t i = 0, j = 0; i < shortestTitle;)
    {
        uint32_t codepointA = 0;
        uint32_t codepointB = 0;

        ssize_t unitCountA = decode_utf8(&codepointA, reinterpret_cast<const uint8_t *>(&titleA[i]));
        ssize_t unitCountB = decode_utf8(&codepointB, reinterpret_cast<const uint8_t *>(&titleB[j]));

        if (unitCountA <= 0 || unitCountB <= 0)
        {
            return false;
        }

        if (codepointA != codepointB)
        {
            return codepointA < codepointB;
        }

        i += unitCountA;
        j += unitCountB;
    }
    return false;
}

// This attempts to create the save data for the given user. It will fail if it already exists.
static void createSaveDataFor(sys::Task *task, data::User *targetUser, data::TitleInfo *titleInfo)
{
    // Set task status.
    task->setStatus(strings::getByName(strings::names::CREATING_SAVE_DATA_FOR, 0), titleInfo->getTitle());

    task->finished();
}

SaveCreateState::SaveCreateState(data::User *targetUser, TitleSelectCommon *titleSelect)
    : m_user(targetUser), m_titleSelect(titleSelect), m_saveMenu(8, 8, 624, 22, 720)
{
    // If the panel is null, create it.
    if (!sm_slidePanel)
    {
        // Create panel and menu.
        sm_slidePanel = std::make_unique<ui::SlideOutPanel>(640, ui::SlideOutPanel::Side::Right);
    }

    // Get title info vector and copy titles to menu.
    data::getTitleInfoByType(m_user->getAccountSaveType(), m_titleInfoVector);

    // Sort it by alpha
    std::sort(m_titleInfoVector.begin(), m_titleInfoVector.end(), compareInfo);

    for (size_t i = 0; i < m_titleInfoVector.size(); i++)
    {
        m_saveMenu.addOption(m_titleInfoVector.at(i)->getTitle());
    }
}

void SaveCreateState::update(void)
{
    sm_slidePanel->update(AppState::hasFocus());
    m_saveMenu.update(AppState::hasFocus());

    if (input::buttonPressed(HidNpadButton_A))
    {
        data::TitleInfo *targetTitle = m_titleInfoVector.at(m_saveMenu.getSelected());
        JKSV::pushState(std::make_shared<TaskState>(createSaveDataFor, m_user, targetTitle));
    }
    else if (input::buttonPressed(HidNpadButton_B))
    {
        sm_slidePanel->close();
    }
    else if (sm_slidePanel->isClosed())
    {
        sm_slidePanel->reset();
        AppState::deactivate();
    }
}

void SaveCreateState::render(void)
{
    // Clear slide target, render menu, render slide to frame buffer.
    sm_slidePanel->clearTarget();
    m_saveMenu.render(sm_slidePanel->get(), AppState::hasFocus());
    sm_slidePanel->render(NULL, AppState::hasFocus());
}

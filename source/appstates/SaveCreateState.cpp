#include "appstates/SaveCreateState.hpp"
#include "JKSV.hpp"
#include "appstates/TaskState.hpp"
#include "data/data.hpp"
#include "fs/fs.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include "system/Task.hpp"
#include "ui/PopMessageManager.hpp"
#include <algorithm>
#include <string>
#include <switch.h>
#include <vector>

// This sorts the vector alphabetically so stuff is easier to find
static bool compare_info(data::TitleInfo *infoA, data::TitleInfo *infoB)
{
    const char *titleA = infoA->get_title();
    const char *titleB = infoB->get_title();

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

// Declarations here. Definitions under class.
static void create_save_data(sys::Task *task, data::User *targetUser, data::TitleInfo *titleInfo);

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
    data::get_title_info_by_type(m_user->get_account_save_type(), m_titleInfoVector);

    // Sort it by alpha
    std::sort(m_titleInfoVector.begin(), m_titleInfoVector.end(), compare_info);

    for (size_t i = 0; i < m_titleInfoVector.size(); i++)
    {
        m_saveMenu.add_option(m_titleInfoVector.at(i)->get_title());
    }
}

void SaveCreateState::update(void)
{
    m_saveMenu.update(AppState::has_focus());
    sm_slidePanel->update(AppState::has_focus());

    if (input::button_pressed(HidNpadButton_A))
    {
        data::TitleInfo *targetTitle = m_titleInfoVector.at(m_saveMenu.get_selected());
        JKSV::push_state(std::make_shared<TaskState>(create_save_data, m_user, targetTitle));
    }
    else if (input::button_pressed(HidNpadButton_B))
    {
        sm_slidePanel->close();
    }
    else if (sm_slidePanel->is_closed())
    {
        sm_slidePanel->reset();
        AppState::deactivate();
    }
}

void SaveCreateState::render(void)
{
    // Clear slide target, render menu, render slide to frame buffer.
    sm_slidePanel->clear_target();
    m_saveMenu.render(sm_slidePanel->get(), AppState::has_focus());
    sm_slidePanel->render(NULL, AppState::has_focus());
}

static void create_save_data(sys::Task *task, data::User *targetUser, data::TitleInfo *titleInfo)
{
    // Set status. We'll just borrow the string from the other group.
    task->set_status(strings::get_by_name(strings::names::USER_OPTION_STATUS, 0), titleInfo->get_title());

    if (fs::create_save_data_for(targetUser, titleInfo))
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_SAVE_CREATE, 0),
                                            titleInfo->get_title());
    }
    else
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_SAVE_CREATE, 1));
    }
    task->finished();
}

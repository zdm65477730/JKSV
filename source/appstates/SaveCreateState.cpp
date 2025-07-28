#include "appstates/SaveCreateState.hpp"

#include "StateManager.hpp"
#include "appstates/TaskState.hpp"
#include "data/data.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "system/Task.hpp"
#include "ui/PopMessageManager.hpp"

#include <algorithm>
#include <string>
#include <switch.h>
#include <vector>

// Declarations here. Definitions under class.
static void create_save_data(sys::Task *task,
                             data::User *targetUser,
                             data::TitleInfo *titleInfo,
                             SaveCreateState *spawningState);

// This is the sorting function.
static bool compare_info(data::TitleInfo *infoA, data::TitleInfo *infoB);

SaveCreateState::SaveCreateState(data::User *user, TitleSelectCommon *titleSelect)
    : m_user{user}
    , m_titleSelect{titleSelect}
    , m_saveMenu{8, 8, 624, 22, 720}
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

    for (size_t i = 0; i < m_titleInfoVector.size(); i++) { m_saveMenu.add_option(m_titleInfoVector.at(i)->get_title()); }
}

void SaveCreateState::update()
{
    const bool hasFocus = BaseState::has_focus();

    if (m_refreshRequired.load())
    {
        m_user->load_user_data();
        m_titleSelect->refresh();
        m_refreshRequired.store(false);
    }

    m_saveMenu.update(hasFocus);
    sm_slidePanel->update(hasFocus);

    if (input::button_pressed(HidNpadButton_A))
    {
        data::TitleInfo *targetTitle = m_titleInfoVector.at(m_saveMenu.get_selected());
        StateManager::push_state(std::make_shared<TaskState>(create_save_data, m_user, targetTitle, this));
    }
    else if (input::button_pressed(HidNpadButton_B)) { sm_slidePanel->close(); }
    else if (sm_slidePanel->is_closed())
    {
        sm_slidePanel->reset();
        BaseState::deactivate();
    }
}

void SaveCreateState::render()
{
    const bool hasFocus = BaseState::has_focus();

    // Clear slide target, render menu, render slide to frame buffer.
    sm_slidePanel->clear_target();
    m_saveMenu.render(sm_slidePanel->get_target(), hasFocus);
    sm_slidePanel->render(NULL, hasFocus);
}

void SaveCreateState::data_and_view_refresh_required() { m_refreshRequired.store(true); }

static void create_save_data(sys::Task *task,
                             data::User *targetUser,
                             data::TitleInfo *titleInfo,
                             SaveCreateState *spawningState)
{
    if (error::is_null(task)) { return; }

    const char *statusTemplate = strings::get_by_name(strings::names::USEROPTION_STATUS, 0);
    const int popTicks         = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popSuccess     = strings::get_by_name(strings::names::SAVECREATE_POPS, 0);
    const char *popFailed      = strings::get_by_name(strings::names::SAVECREATE_POPS, 1);

    {
        const std::string status = stringutil::get_formatted_string(statusTemplate, titleInfo->get_title());
        task->set_status(status);
    }

    const bool created = fs::create_save_data_for(targetUser, titleInfo);
    if (created)
    {
        const char *title      = titleInfo->get_title();
        std::string popMessage = stringutil::get_formatted_string(popSuccess, title);
        ui::PopMessageManager::push_message(popTicks, popMessage);
    }
    else { ui::PopMessageManager::push_message(popTicks, popFailed); }

    spawningState->data_and_view_refresh_required();
    task->finished();
}

static bool compare_info(data::TitleInfo *infoA, data::TitleInfo *infoB)
{
    // Get pointers to both titles.
    const char *titleA = infoA->get_title();
    const char *titleB = infoB->get_title();

    size_t titleALength  = std::char_traits<char>::length(titleA);
    size_t titleBLength  = std::char_traits<char>::length(titleB);
    size_t shortestTitle = titleALength < titleBLength ? titleALength : titleBLength;
    // To do: This doesn't take into account which is the shortest title. This can still go out-of-bounds.
    for (size_t i = 0, j = 0; i < shortestTitle;)
    {
        uint32_t codepointA = 0;
        uint32_t codepointB = 0;

        ssize_t unitCountA = decode_utf8(&codepointA, reinterpret_cast<const uint8_t *>(&titleA[i]));
        ssize_t unitCountB = decode_utf8(&codepointB, reinterpret_cast<const uint8_t *>(&titleB[j]));

        if (unitCountA <= 0 || unitCountB <= 0) { return false; }

        if (codepointA != codepointB) { return codepointA < codepointB; }

        i += unitCountA;
        j += unitCountB;
    }
    return false;
}

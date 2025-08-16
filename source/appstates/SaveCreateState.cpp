#include "appstates/SaveCreateState.hpp"

#include "StateManager.hpp"
#include "appstates/TaskState.hpp"
#include "data/data.hpp"
#include "fs/fs.hpp"
#include "input.hpp"
#include "logging/error.hpp"
#include "logging/logger.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
#include "tasks/savecreate.hpp"
#include "ui/PopMessageManager.hpp"

#include <algorithm>
#include <string>
#include <switch.h>
#include <vector>

// This is the sorting function.
static bool compare_info(data::TitleInfo *infoA, data::TitleInfo *infoB);

SaveCreateState::SaveCreateState(data::User *user, TitleSelectCommon *titleSelect)
    : m_user(user)
    , m_titleSelect(titleSelect)
    , m_saveMenu(ui::Menu::create(8, 8, 624, 22, 720))
{
    SaveCreateState::initialize_static_members();
    SaveCreateState::initialize_title_info_vector();
    SaveCreateState::initialize_menu();
}

SaveCreateState::~SaveCreateState()
{
    sm_slidePanel->clear_elements();
    sm_slidePanel->reset();
}

void SaveCreateState::update()
{
    const bool hasFocus = BaseState::has_focus();

    sm_slidePanel->update(hasFocus);

    const bool aPressed    = input::button_pressed(HidNpadButton_A);
    const bool bPressed    = input::button_pressed(HidNpadButton_B);
    const bool panelClosed = sm_slidePanel->is_closed();

    if (m_refreshRequired.load())
    {
        m_user->load_user_data();
        m_titleSelect->refresh();
        m_refreshRequired.store(false);
    }

    if (aPressed) { SaveCreateState::create_save_data_for(); }
    else if (bPressed) { sm_slidePanel->close(); }
    else if (panelClosed) { BaseState::deactivate(); }
}

void SaveCreateState::render()
{
    const bool hasFocus = BaseState::has_focus();

    // Clear slide target, render menu, render slide to frame buffer.
    sm_slidePanel->clear_target();
    sm_slidePanel->render(sdl::Texture::Null, hasFocus);
}

void SaveCreateState::refresh_required() { m_refreshRequired.store(true); }

void SaveCreateState::initialize_static_members()
{
    if (!sm_slidePanel) { sm_slidePanel = std::make_unique<ui::SlideOutPanel>(640, ui::SlideOutPanel::Side::Right); }
}

void SaveCreateState::initialize_title_info_vector()
{
    const FsSaveDataType saveType = m_user->get_account_save_type();
    data::get_title_info_by_type(saveType, m_titleInfoVector);

    std::sort(m_titleInfoVector.begin(), m_titleInfoVector.end(), compare_info);
}

void SaveCreateState::initialize_menu()
{
    for (data::TitleInfo *titleInfo : m_titleInfoVector)
    {
        const std::string_view title = titleInfo->get_title();
        m_saveMenu->add_option(title);
    }
    sm_slidePanel->push_new_element(m_saveMenu);
}

void SaveCreateState::create_save_data_for()
{
    const int selected         = m_saveMenu->get_selected();
    data::TitleInfo *titleInfo = m_titleInfoVector[selected];
    auto createTask            = TaskState::create(tasks::savecreate::create_save_data_for, m_user, titleInfo, this);

    StateManager::push_state(createTask);
}

static bool compare_info(data::TitleInfo *infoA, data::TitleInfo *infoB)
{
    // Get pointers to both titles.
    const char *titleA = infoA->get_title();
    const char *titleB = infoB->get_title();

    const size_t titleALength  = std::char_traits<char>::length(titleA);
    const size_t titleBLength  = std::char_traits<char>::length(titleB);
    const size_t shortestTitle = titleALength < titleBLength ? titleALength : titleBLength;
    // To do: This doesn't take into account which is the shortest title. This can still go out-of-bounds.
    for (size_t i = 0, j = 0; i < shortestTitle;)
    {
        uint32_t codepointA = 0;
        uint32_t codepointB = 0;

        const uint8_t *pointA    = reinterpret_cast<const uint8_t *>(&titleA[i]);
        const uint8_t *pointB    = reinterpret_cast<const uint8_t *>(&titleB[j]);
        const ssize_t unitCountA = decode_utf8(&codepointA, pointA);
        const ssize_t unitCountB = decode_utf8(&codepointB, pointB);
        if (unitCountA <= 0 || unitCountB <= 0) { return false; }

        if (codepointA != codepointB) { return codepointA < codepointB; }

        i += unitCountA;
        j += unitCountB;
    }
    return false;
}

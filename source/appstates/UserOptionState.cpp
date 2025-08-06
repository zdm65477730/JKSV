#include "appstates/UserOptionState.hpp"

#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/SaveCreateState.hpp"
#include "appstates/TaskState.hpp"
#include "config.hpp"
#include "data/data.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "remote/remote.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
#include "tasks/useroptions.hpp"
#include "ui/PopMessageManager.hpp"

namespace
{
    // Enum for menu switch case.
    enum
    {
        BACKUP_ALL,
        CREATE_SAVE,
        CREATE_ALL_SAVE,
        DELETE_ALL_SAVE
    };

    using TaskConfirm     = ConfirmState<sys::Task, TaskState, UserOptionState::DataStruct>;
    using ProgressConfirm = ConfirmState<sys::ProgressTask, ProgressState, UserOptionState::DataStruct>;

} // namespace

UserOptionState::UserOptionState(data::User *user, TitleSelectCommon *titleSelect)
    : m_user(user)
    , m_titleSelect(titleSelect)
    , m_userOptionMenu(8, 8, 460, 22, 720)
    , m_dataStruct(std::make_shared<UserOptionState::DataStruct>())
{
    UserOptionState::create_menu_panel();
    UserOptionState::load_menu_strings();
    UserOptionState::initialize_data_struct();
}

std::shared_ptr<UserOptionState> UserOptionState::create(data::User *user, TitleSelectCommon *titleSelect)
{
    return std::make_shared<UserOptionState>(user, titleSelect);
}

std::shared_ptr<UserOptionState> UserOptionState::create_and_push(data::User *user, TitleSelectCommon *titleSelect)
{
    auto newState = UserOptionState::create(user, titleSelect);
    StateManager::push_state(newState);
    return newState;
}

void UserOptionState::update()
{
    const bool hasFocus = BaseState::has_focus();
    sm_menuPanel->update(hasFocus);

    const bool isOpen = sm_menuPanel->is_open();
    if (!isOpen) { return; }

    const bool aPressed = input::button_pressed(HidNpadButton_A);
    const bool bPressed = input::button_pressed(HidNpadButton_B);

    // See if this needs to be done.
    if (m_refreshRequired)
    {
        m_user->load_user_data();
        m_titleSelect->refresh();
        m_refreshRequired = false;
    }

    if (aPressed)
    {
        const int selected = m_userOptionMenu.get_selected();

        switch (selected)
        {
            case BACKUP_ALL:      UserOptionState::backup_all(); break;
            case CREATE_SAVE:     UserOptionState::create_save_create(); break;
            case CREATE_ALL_SAVE: UserOptionState::create_all_save_data(); break;
            case DELETE_ALL_SAVE: UserOptionState::delete_all_save_data(); break;
        }
    }
    else if (bPressed) { sm_menuPanel->close(); }
    else if (sm_menuPanel->is_closed())
    {
        BaseState::deactivate();
        sm_menuPanel->reset();
    }

    m_userOptionMenu.update(BaseState::has_focus());
}

void UserOptionState::render()
{
    // Render target user's title selection screen.
    m_titleSelect->render();
    sdl::SharedTexture &panelTarget = sm_menuPanel->get_target();

    // Render panel.
    sm_menuPanel->clear_target();
    m_userOptionMenu.render(panelTarget, BaseState::has_focus());
    sm_menuPanel->render(sdl::Texture::Null, BaseState::has_focus());
}

void UserOptionState::refresh_required() { m_refreshRequired = true; }

void UserOptionState::create_menu_panel()
{
    static constexpr int SIZE_PANEL_WIDTH = 480;
    if (!sm_menuPanel) { sm_menuPanel = std::make_unique<ui::SlideOutPanel>(SIZE_PANEL_WIDTH, ui::SlideOutPanel::Side::Right); }
}

void UserOptionState::load_menu_strings()
{
    const char *nickname = m_user->get_nickname();

    for (int i = 0; const char *format = strings::get_by_name(strings::names::USEROPTION_MENU, i); i++)
    {
        const std::string option = stringutil::get_formatted_string(format, nickname);
        m_userOptionMenu.add_option(option);
    }
}

void UserOptionState::initialize_data_struct()
{
    m_dataStruct->user          = m_user;
    m_dataStruct->spawningState = this;
}

void UserOptionState::backup_all()
{
    remote::Storage *remote   = remote::get_remote_storage();
    bool autoUpload           = config::get_by_key(config::keys::AUTO_UPLOAD);
    const char *confirmFormat = strings::get_by_name(strings::names::USEROPTION_CONFS, 0);
    const char *nickname      = m_user->get_nickname();

    const std::string query = stringutil::get_formatted_string(confirmFormat, nickname);
    if (remote && autoUpload)
    {
        ProgressConfirm::create_and_push(query, true, tasks::useroptions::backup_all_for_user_remote, m_dataStruct);
    }
    else { ProgressConfirm::create_and_push(query, true, tasks::useroptions::backup_all_for_user_local, m_dataStruct); }
}

void UserOptionState::create_save_create() { SaveCreateState::create_and_push(m_user, m_titleSelect); }

void UserOptionState::create_all_save_data()
{
    const char *confirmFormat = strings::get_by_name(strings::names::USEROPTION_CONFS, 1);
    const char *nickname      = m_user->get_nickname();

    const std::string query = stringutil::get_formatted_string(confirmFormat, nickname);
    TaskConfirm::create_and_push(query, true, tasks::useroptions::create_all_save_data_for_user, m_dataStruct);
}

void UserOptionState::delete_all_save_data()
{
    const uint8_t saveType = m_user->get_account_save_type();
    if (saveType == FsSaveDataType_System || saveType == FsSaveDataType_SystemBcat) { return; }

    const char *confirmFormat     = strings::get_by_name(strings::names::USEROPTION_CONFS, 2);
    const char *nickname          = m_user->get_nickname();
    const std::string queryString = stringutil::get_formatted_string(confirmFormat, nickname);
    TaskConfirm::create_and_push(queryString, true, tasks::useroptions::delete_all_save_data_for_user, m_dataStruct);
}

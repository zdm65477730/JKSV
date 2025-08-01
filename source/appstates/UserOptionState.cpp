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
#include "strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
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

// Declarations here. Defintions after class.
// Backs up all save data for the target user.
static void backup_all_for_user(sys::ProgressTask *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct);
// // Creates all save data for the current user.
static void create_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct);
// // Deletes all save data from the system for the target user.
static void delete_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct);

UserOptionState::UserOptionState(data::User *user, TitleSelectCommon *titleSelect)
    : m_user{user}
    , m_titleSelect{titleSelect}
    , m_userOptionMenu{8, 8, 460, 22, 720}
    , m_dataStruct{std::make_shared<UserOptionState::DataStruct>()}
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
    const bool aPressed = input::button_pressed(HidNpadButton_A);
    const bool bPressed = input::button_pressed(HidNpadButton_B);

    sm_menuPanel->update(hasFocus);

    // See if this needs to be done.
    if (m_refreshRequired)
    {
        m_user->load_user_data();
        m_titleSelect->refresh();
        m_refreshRequired = false;
    }

    if (aPressed)
    {
        switch (m_userOptionMenu.get_selected())
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

    // Render panel.
    sm_menuPanel->clear_target();
    m_userOptionMenu.render(sm_menuPanel->get_target(), BaseState::has_focus());
    sm_menuPanel->render(NULL, BaseState::has_focus());
}

void UserOptionState::data_and_view_refresh_required() { m_refreshRequired = true; }

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
    const char *confirmFormat = strings::get_by_name(strings::names::USEROPTION_CONFS, 0);
    const char *nickname      = m_user->get_nickname();

    const std::string queryString = stringutil::get_formatted_string(confirmFormat, nickname);

    auto confirm = std::make_shared<ProgressConfirm>(queryString, true, backup_all_for_user, m_dataStruct);
    StateManager::push_state(confirm);
}

void UserOptionState::create_save_create()
{
    auto saveCreate = std::make_shared<SaveCreateState>(m_user, m_titleSelect);
    StateManager::push_state(saveCreate);
}

void UserOptionState::create_all_save_data()
{
    const char *confirmFormat = strings::get_by_name(strings::names::USEROPTION_CONFS, 1);
    const char *nickname      = m_user->get_nickname();

    const std::string queryString = stringutil::get_formatted_string(confirmFormat, nickname);

    auto confirm = std::make_shared<TaskConfirm>(queryString, true, create_all_save_data_for_user, m_dataStruct);
    StateManager::push_state(confirm);
}

void UserOptionState::delete_all_save_data()
{
    const uint8_t saveType = m_user->get_account_save_type();
    if (saveType == FsSaveDataType_System || saveType == FsSaveDataType_SystemBcat) { return; }

    const char *confirmFormat = strings::get_by_name(strings::names::USEROPTION_CONFS, 2);
    const char *nickname      = m_user->get_nickname();

    const std::string queryString = stringutil::get_formatted_string(confirmFormat, nickname);

    auto confirm = std::make_shared<TaskConfirm>(queryString, true, delete_all_save_data_for_user, m_dataStruct);
    StateManager::push_state(confirm);
}

static void backup_all_for_user(sys::ProgressTask *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct)
{
    if (error::is_null(task)) { return; }

    data::User *user = dataStruct->user;

    const bool exportZip    = config::get_by_key(config::keys::EXPORT_TO_ZIP) || config::get_by_key(config::keys::AUTO_UPLOAD);
    const bool autoUpload   = config::get_by_key(config::keys::AUTO_UPLOAD);
    const size_t titleCount = user->get_total_data_entries();
    for (size_t i = 0; i < titleCount; i++)
    {
        const FsSaveDataInfo *saveInfo = user->get_save_info_at(i);
        data::TitleInfo *titleInfo     = data::get_title_info_by_id(saveInfo->application_id);
        if (error::is_null(saveInfo) || error::is_null(titleInfo)) { continue; }

        const fslib::Path targetDir{config::get_working_directory() / titleInfo->get_path_safe_title()};
        const bool targetExists = fslib::directory_exists(targetDir);
        const bool targetFailed = !targetExists && error::fslib(fslib::create_directories_recursively(targetDir));
        if (!targetExists && targetFailed) { continue; }

        const bool mountFailed = error::fslib(fslib::open_save_data_with_save_info(fs::DEFAULT_SAVE_MOUNT, *saveInfo));
        const bool validSave   = !mountFailed && fs::directory_has_contents(fs::DEFAULT_SAVE_ROOT);
        if (mountFailed || !validSave)
        {
            fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);
            continue;
        }

        fs::SaveMetaData saveMeta{};
        const bool hasMeta = fs::fill_save_meta_data(saveInfo, saveMeta);

        fslib::Path backupPath{targetDir / "AUTO - " + user->get_path_safe_nickname() + " - " + stringutil::get_date_string()};
        if (exportZip)
        {
            backupPath += ".zip";
            fs::MiniZip targetZip{backupPath};
            if (!targetZip.is_open())
            {
                fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);
                continue;
            }

            if (hasMeta && targetZip.open_new_file(fs::NAME_SAVE_META))
            {
                targetZip.write(&saveMeta, sizeof(fs::SaveMetaData));
                targetZip.close_current_file();
            }
            fs::copy_directory_to_zip(fs::DEFAULT_SAVE_ROOT, targetZip, task);
        }
        else
        {
            {
                fslib::File metaFile{backupPath / fs::NAME_SAVE_META, FsOpenMode_Create | FsOpenMode_Write};
                if (hasMeta && metaFile.is_open()) { metaFile.write(&saveMeta, sizeof(fs::SaveMetaData)); }
                fs::copy_directory(fs::DEFAULT_SAVE_ROOT, backupPath, task);
            }
        }
    }
    task->finished();
}

static void create_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct)
{
    if (error::is_null(task)) { return; }

    data::User *user               = dataStruct->user;
    UserOptionState *spawningState = dataStruct->spawningState;

    auto &titleInfoMap         = data::get_title_info_map();
    const int popTicks         = ui::PopMessageManager::DEFAULT_TICKS;
    const char *statusTemplate = strings::get_by_name(strings::names::USEROPTION_STATUS, 0);
    const char *popFailure     = strings::get_by_name(strings::names::SAVECREATE_POPS, 1);
    const uint8_t saveType     = user->get_account_save_type();

    for (auto &[applicationID, titleInfo] : titleInfoMap)
    {

        const bool hasType = titleInfo.has_save_data_type(saveType);
        if (!hasType) { continue; }

        {
            const std::string status = stringutil::get_formatted_string(statusTemplate, titleInfo.get_title());
            task->set_status(status);
        }

        const bool saveCreated = fs::create_save_data_for(user, &titleInfo);
        if (!saveCreated)
        {
            const char *title     = titleInfo.get_title();
            const std::string pop = stringutil::get_formatted_string(popFailure, title);
            ui::PopMessageManager::push_message(popTicks, pop);
        }
    }
    spawningState->data_and_view_refresh_required();
    task->finished();
}

static void delete_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct)
{
    if (error::is_null(task)) { return; }

    data::User *user               = dataStruct->user;
    UserOptionState *spawningState = dataStruct->spawningState;
    const char *statusTemplate     = strings::get_by_name(strings::names::USEROPTION_STATUS, 1); // Borrowed. No duplication.
    const char *popFailed          = strings::get_by_name(strings::names::SAVECREATE_POPS, 2);
    const int popTicks             = ui::PopMessageManager::DEFAULT_TICKS;
    const size_t totalDataEntries  = user->get_total_data_entries();
    std::vector<uint64_t> applicationIDs;

    // Check this quick just in case.
    if (user->get_account_save_type() == FsSaveDataType_System) { TASK_FINISH_RETURN(task); }

    for (size_t i = 0; i < totalDataEntries; i++)
    {
        const FsSaveDataInfo *saveInfo = user->get_save_info_at(i);
        if (error::is_null(saveInfo) || saveInfo->save_data_type == FsSaveDataType_System) { continue; }

        {
            const uint64_t applicationID = user->get_application_id_at(i);
            data::TitleInfo *titleInfo   = data::get_title_info_by_id(applicationID);
            const char *title            = titleInfo->get_title();
            const std::string status     = stringutil::get_formatted_string(statusTemplate, title);
            task->set_status(status);
        }

        const bool saveDeleted = fs::delete_save_data(saveInfo);
        if (!saveDeleted)
        {
            ui::PopMessageManager::push_message(popTicks, popFailed);
            continue;
        }
        applicationIDs.push_back(saveInfo->application_id);
    }

    for (uint64_t &applicationID : applicationIDs) { user->erase_save_info_by_id(applicationID); }
    spawningState->data_and_view_refresh_required();
    task->finished();
}

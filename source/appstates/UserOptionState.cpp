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
#include "system/system.hpp"
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
} // namespace

// Declarations here. Defintions after class.
// Backs up all save data for the target user.
static void backup_all_for_user(sys::ProgressTask *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct);
// // Creates all save data for the current user.
static void create_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct);
// // Deletes all save data from the system for the target user.
static void delete_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct);

UserOptionState::UserOptionState(data::User *user, TitleSelectCommon *titleSelect)
    : m_user(user)
    , m_titleSelect(titleSelect)
    , m_userOptionMenu(8, 8, 460, 22, 720)
    , m_dataStruct(std::make_shared<UserOptionState::DataStruct>())
{
    // Check if panel needs to be created. It's shared by all instances.
    if (!m_menuPanel) { m_menuPanel = std::make_unique<ui::SlideOutPanel>(480, ui::SlideOutPanel::Side::Right); }

    int currentStringIndex    = 0;
    const char *currentString = nullptr;
    while ((currentString = strings::get_by_name(strings::names::USEROPTION_MENU, currentStringIndex++)) != nullptr)
    {
        m_userOptionMenu.add_option(stringutil::get_formatted_string(currentString, m_user->get_nickname()));
    }

    // Fill this is.
    m_dataStruct->user          = m_user;
    m_dataStruct->spawningState = this;
}

void UserOptionState::update()
{
    // Update the main panel.
    m_menuPanel->update(BaseState::has_focus());

    // See if this needs to be done.
    if (m_refreshRequired)
    {
        m_user->load_user_data();
        m_titleSelect->refresh();
        m_refreshRequired = false;
    }

    if (input::button_pressed(HidNpadButton_A) && m_user->get_account_save_type() != FsSaveDataType_System)
    {
        switch (m_userOptionMenu.get_selected())
        {
            case BACKUP_ALL:
            {
                // This is broken down to make it easier to read.
                std::string queryString =
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::USEROPTION_CONFS, 0),
                                                     m_user->get_nickname());

                // State to push
                auto confirmBackupAll =
                    std::make_shared<ConfirmState<sys::ProgressTask, ProgressState, UserOptionState::DataStruct>>(
                        queryString,
                        false,
                        backup_all_for_user,
                        m_dataStruct);

                StateManager::push_state(confirmBackupAll);
            }
            break;

            case CREATE_SAVE:
            {
                auto saveCreateState = std::make_shared<SaveCreateState>(m_user, m_titleSelect);

                // This just pushes the state with the menu to select.
                StateManager::push_state(std::make_shared<SaveCreateState>(m_user, m_titleSelect));
            }
            break;

            case CREATE_ALL_SAVE:
            {
                std::string queryString =
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::USEROPTION_CONFS, 1),
                                                     m_user->get_nickname());

                auto confirmCreateAll = std::make_shared<ConfirmState<sys::Task, TaskState, UserOptionState::DataStruct>>(
                    queryString,
                    true,
                    create_all_save_data_for_user,
                    m_dataStruct);

                // Done?
                StateManager::push_state(confirmCreateAll);
            }
            break;

            case DELETE_ALL_SAVE:
            {
                std::string queryString =
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::USEROPTION_CONFS, 2),
                                                     m_user->get_nickname());

                auto confirmDeleteAll = std::make_shared<ConfirmState<sys::Task, TaskState, UserOptionState::DataStruct>>(
                    queryString,
                    true,
                    delete_all_save_data_for_user,
                    m_dataStruct);

                StateManager::push_state(confirmDeleteAll);
            }
            break;
        }
    }
    else if (input::button_pressed(HidNpadButton_B)) { m_menuPanel->close(); }
    else if (m_menuPanel->is_closed())
    {
        BaseState::deactivate();
        m_menuPanel->reset();
    }

    m_userOptionMenu.update(BaseState::has_focus());
}

void UserOptionState::render()
{
    // Render target user's title selection screen.
    m_titleSelect->render();

    // Render panel.
    m_menuPanel->clear_target();
    m_userOptionMenu.render(m_menuPanel->get_target(), BaseState::has_focus());
    m_menuPanel->render(NULL, BaseState::has_focus());
}

void UserOptionState::data_and_view_refresh_required() { m_refreshRequired = true; }

static void backup_all_for_user(sys::ProgressTask *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct)
{
    if (error::is_null(task)) { return; }

    data::User *user               = dataStruct->user;
    UserOptionState *spawningState = dataStruct->spawningState;

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

    auto &titleInfoMap            = data::get_title_info_map();
    const int popTicks            = ui::PopMessageManager::DEFAULT_TICKS;
    const char *statusTemplate    = strings::get_by_name(strings::names::USEROPTION_STATUS, 0);
    const char *popFailure        = strings::get_by_name(strings::names::SAVECREATE_POPS, 0);
    const FsSaveDataType saveType = user->get_account_save_type();

    for (auto &[applicationID, titleInfo] : titleInfoMap)
    {
        const bool hasType = titleInfo.has_save_data_type(saveType);
        if (!hasType) { return; }

        {
            const std::string status = stringutil::get_formatted_string(statusTemplate, titleInfo.get_title());
            task->set_status(status);
        }

        const bool saveCreated = fs::create_save_data_for(user, &titleInfo);
        if (!saveCreated) { ui::PopMessageManager::push_message(popTicks, popFailure); }
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
    if (user->get_account_save_type() == FsSaveDataType_System)
    {
        task->finished();
        return;
    }

    for (size_t i = 0; i < totalDataEntries; i++)
    {
        const FsSaveDataInfo *saveInfo = user->get_save_info_at(i);
        if (error::is_null(saveInfo)) { continue; }

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

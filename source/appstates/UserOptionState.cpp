#include "appstates/UserOptionState.hpp"
#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/SaveCreateState.hpp"
#include "appstates/TaskState.hpp"
#include "config.hpp"
#include "data/data.hpp"
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
    : m_user(user), m_titleSelect(titleSelect), m_userOptionMenu(8, 8, 460, 22, 720),
      m_dataStruct(std::make_shared<UserOptionState::DataStruct>())
{
    // Check if panel needs to be created. It's shared by all instances.
    if (!m_menuPanel)
    {
        m_menuPanel = std::make_unique<ui::SlideOutPanel>(480, ui::SlideOutPanel::Side::Right);
    }

    int currentStringIndex = 0;
    const char *currentString = nullptr;
    while ((currentString = strings::get_by_name(strings::names::USER_OPTIONS, currentStringIndex++)) != nullptr)
    {
        m_userOptionMenu.add_option(stringutil::get_formatted_string(currentString, m_user->get_nickname()));
    }

    // Fill this is.
    m_dataStruct->m_user = m_user;
    m_dataStruct->m_spawningState = this;
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
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::USER_OPTION_CONFIRMATIONS, 0),
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
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::USER_OPTION_CONFIRMATIONS, 1),
                                                     m_user->get_nickname());

                auto confirmCreateAll =
                    std::make_shared<ConfirmState<sys::Task, TaskState, UserOptionState::DataStruct>>(
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
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::USER_OPTION_CONFIRMATIONS, 2),
                                                     m_user->get_nickname());

                auto confirmDeleteAll =
                    std::make_shared<ConfirmState<sys::Task, TaskState, UserOptionState::DataStruct>>(
                        queryString,
                        true,
                        delete_all_save_data_for_user,
                        m_dataStruct);

                StateManager::push_state(confirmDeleteAll);
            }
            break;
        }
    }
    else if (input::button_pressed(HidNpadButton_B))
    {
        m_menuPanel->close();
    }
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

void UserOptionState::data_and_view_refresh_required()
{
    m_refreshRequired = true;
}

static void backup_all_for_user(sys::ProgressTask *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct)
{
    data::User *targetUser = dataStruct->m_user;

    for (size_t i = 0; i < targetUser->get_total_data_entries(); i++)
    {
        // This should be safe like this....
        FsSaveDataInfo *currentSaveInfo = targetUser->get_save_info_at(i);
        data::TitleInfo *currentTitle = data::get_title_info_by_id(currentSaveInfo->application_id);

        if (!currentSaveInfo || !currentTitle)
        {
            continue;
        }

        // Try to create target game folder.
        fslib::Path gameFolder = config::get_working_directory() / currentTitle->get_path_safe_title();
        if (!fslib::directory_exists(gameFolder) && !fslib::create_directory(gameFolder))
        {
            continue;
        }

        // Try to mount save data.
        bool saveMounted =
            fslib::open_save_data_with_save_info(fs::DEFAULT_SAVE_MOUNT, *targetUser->get_save_info_at(i));

        // Check to make sure the save actually has data to avoid blanks.
        {
            fslib::Directory saveCheck(fs::DEFAULT_SAVE_ROOT);
            if (saveMounted && saveCheck.get_count() <= 0)
            {
                // Gonna borrow these messages. No point in repeating them.
                ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                    strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 0));
                fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);
                continue;
            }
        }

        if (currentTitle && saveMounted && config::get_by_key(config::keys::EXPORT_TO_ZIP))
        {
            fslib::Path targetPath = config::get_working_directory() / currentTitle->get_path_safe_title() /
                                         targetUser->get_path_safe_nickname() +
                                     " - " + stringutil::get_date_string() + ".zip";

            zipFile targetZip = zipOpen64(targetPath.full_path(), APPEND_STATUS_CREATE);
            if (!targetZip)
            {
                logger::log("Error creating zip: %s", fslib::error::get_string());
                continue;
            }
            fs::copy_directory_to_zip(fs::DEFAULT_SAVE_ROOT, targetZip, task);
            zipClose(targetZip, NULL);
        }
        else if (currentTitle && saveMounted)
        {
            fslib::Path targetPath = config::get_working_directory() / currentTitle->get_path_safe_title() /
                                         targetUser->get_path_safe_nickname() +
                                     " - " + stringutil::get_date_string();

            if (!fslib::create_directory(targetPath))
            {
                logger::log("Error creating backup directory: %s", fslib::error::get_string());
                continue;
            }
            fs::copy_directory(fs::DEFAULT_SAVE_ROOT, targetPath, 0, {}, task);
        }

        if (saveMounted)
        {
            fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);
        }
    }
    task->finished();
}

static void create_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct)
{
    data::User *targetUser = dataStruct->m_user;

    // Get title info map.
    auto &titleInfoMap = data::get_title_info_map();

    // Iterate through it.
    for (auto &[applicationID, titleInfo] : titleInfoMap)
    {
        if (!titleInfo.has_save_data_type(targetUser->get_account_save_type()))
        {
            continue;
        }

        // Set status.
        task->set_status(strings::get_by_name(strings::names::USER_OPTION_STATUS, 0), titleInfo.get_title());

        if (!fs::create_save_data_for(targetUser, &titleInfo))
        {
            // Function should log error.
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::POP_MESSAGES_SAVE_CREATE, 0));
        }
    }

    // This needs to be updated on the next update loop.
    dataStruct->m_spawningState->data_and_view_refresh_required();

    task->finished();
}

static void delete_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserOptionState::DataStruct> dataStruct)
{
    // This just makes things easier to type.
    data::User *targetUser = dataStruct->m_user;

    // This is to keep track of what's deleted. Erasing on every loop throws the vector out of whack.
    std::vector<uint64_t> applicationIDs;

    for (size_t i = 0; i < targetUser->get_total_data_entries(); i++)
    {
        // Grab title for title.
        const char *targetTitle = data::get_title_info_by_id(targetUser->get_application_id_at(i))->get_title();

        // Update thread task.
        task->set_status(strings::get_by_name(strings::names::USER_OPTION_STATUS, 1), targetTitle);

        // Grab a pointer quick.
        FsSaveDataInfo *saveInfo = targetUser->get_save_info_at(i);

        // We don't want to let people nuke their entire system, basically.
        if (saveInfo->save_data_type != FsSaveDataType_System && !fs::delete_save_data(targetUser->get_save_info_at(i)))
        {
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::POP_MESSAGES_SAVE_CREATE, 2));
            continue;
        }

        // Push the application ID back.
        applicationIDs.push_back(saveInfo->application_id);
    }

    // Loop through the IDs and purge them all.
    for (uint64_t &applicationID : applicationIDs)
    {
        targetUser->erase_save_info_by_id(applicationID);
    }

    // Signal the main thread to update~
    dataStruct->m_spawningState->data_and_view_refresh_required();

    task->finished();
}

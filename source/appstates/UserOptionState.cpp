#include "appstates/UserOptionState.hpp"
#include "JKSV.hpp"
#include "appstates/ConfirmState.hpp"
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

// Struct to pass data to functions that require it.
typedef struct
{
        data::User *m_targetUser;
} UserStruct;

// Declarations here. Defintions after class.
// Backs up all save data for the target user.
static void backup_all_for_user(sys::ProgressTask *task, std::shared_ptr<UserStruct> dataStruct);
// // Creates all save data for the current user.
static void create_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserStruct> dataStruct);
// // Deletes all save data from the system for the target user.
static void delete_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserStruct> dataStruct);

UserOptionState::UserOptionState(data::User *user, TitleSelectCommon *titleSelect)
    : m_user(user), m_titleSelect(titleSelect), m_userOptionMenu(8, 8, 460, 22, 720)
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
}

void UserOptionState::update(void)
{
    m_menuPanel->update(AppState::has_focus());

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

                // Data to send if confirmed.
                std::shared_ptr<UserStruct> dataStruct(new UserStruct);
                dataStruct->m_targetUser = m_user;

                // State to push
                auto confirmBackupAll =
                    std::make_shared<ConfirmState<sys::ProgressTask, ProgressState, UserStruct>>(queryString,
                                                                                                 false,
                                                                                                 backup_all_for_user,
                                                                                                 dataStruct);

                JKSV::push_state(confirmBackupAll);
            }
            break;

            case CREATE_SAVE:
            {
                // This just pushes the state with the menu to select.
                JKSV::push_state(std::make_shared<SaveCreateState>(m_user, m_titleSelect));
            }
            break;

            case CREATE_ALL_SAVE:
            {
                std::string queryString =
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::USER_OPTION_CONFIRMATIONS, 1),
                                                     m_user->get_nickname());

                std::shared_ptr<UserStruct> dataStruct(new UserStruct);
                dataStruct->m_targetUser = m_user;

                auto confirmCreateAll =
                    std::make_shared<ConfirmState<sys::Task, TaskState, UserStruct>>(queryString,
                                                                                     true,
                                                                                     create_all_save_data_for_user,
                                                                                     dataStruct);

                // Done?
                JKSV::push_state(confirmCreateAll);
            }
            break;

            case DELETE_ALL_SAVE:
            {
                std::string queryString =
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::USER_OPTION_CONFIRMATIONS, 2),
                                                     m_user->get_nickname());

                std::shared_ptr<UserStruct> dataStruct(new UserStruct);
                dataStruct->m_targetUser = m_user;

                auto confirmDeleteAll =
                    std::make_shared<ConfirmState<sys::Task, TaskState, UserStruct>>(queryString,
                                                                                     true,
                                                                                     delete_all_save_data_for_user,
                                                                                     dataStruct);

                JKSV::push_state(confirmDeleteAll);
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
        AppState::deactivate();
        m_menuPanel->reset();
    }

    m_userOptionMenu.update(AppState::has_focus());
}

void UserOptionState::render(void)
{
    // Render target user's title selection screen.
    m_titleSelect->render();

    // Render panel.
    m_menuPanel->clear_target();
    m_userOptionMenu.render(m_menuPanel->get(), AppState::has_focus());
    m_menuPanel->render(NULL, AppState::has_focus());
}

static void backup_all_for_user(sys::ProgressTask *task, std::shared_ptr<UserStruct> dataStruct)
{
    data::User *targetUser = dataStruct->m_targetUser;

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
        if (!fslib::directoryExists(gameFolder) && !fslib::createDirectory(gameFolder))
        {
            continue;
        }

        // Try to mount save data.
        bool saveMounted =
            fslib::openSaveFileSystemWithSaveDataInfo(fs::DEFAULT_SAVE_MOUNT, *targetUser->get_save_info_at(i));

        // Check to make sure the save actually has data to avoid blanks.
        {
            fslib::Directory saveCheck(fs::DEFAULT_SAVE_PATH);
            if (saveMounted && saveCheck.getCount() <= 0)
            {
                // Gonna borrow these messages. No point in repeating them.
                ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                    strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 0));
                fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);
                continue;
            }
        }

        if (currentTitle && saveMounted && config::get_by_key(config::keys::EXPORT_TO_ZIP))
        {
            fslib::Path targetPath = config::get_working_directory() / currentTitle->get_path_safe_title() /
                                         targetUser->get_path_safe_nickname() +
                                     " - " + stringutil::get_date_string() + ".zip";

            zipFile targetZip = zipOpen64(targetPath.cString(), APPEND_STATUS_CREATE);
            if (!targetZip)
            {
                logger::log("Error creating zip: %s", fslib::getErrorString());
                continue;
            }
            fs::copy_directory_to_zip(fs::DEFAULT_SAVE_PATH, targetZip, task);
            zipClose(targetZip, NULL);
        }
        else if (currentTitle && saveMounted)
        {
            fslib::Path targetPath = config::get_working_directory() / currentTitle->get_path_safe_title() /
                                         targetUser->get_path_safe_nickname() +
                                     " - " + stringutil::get_date_string();

            if (!fslib::createDirectory(targetPath))
            {
                logger::log("Error creating backup directory: %s", fslib::getErrorString());
                continue;
            }
            fs::copy_directory(fs::DEFAULT_SAVE_PATH, targetPath, 0, {}, task);
        }

        if (saveMounted)
        {
            fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);
        }
    }
    task->finished();
}

static void create_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserStruct> dataStruct)
{
    data::User *targetUser = dataStruct->m_targetUser;

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
            // Function should log error too.
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::POP_MESSAGES_SAVE_CREATE, 2));
        }
    }
    task->finished();
}

static void delete_all_save_data_for_user(sys::Task *task, std::shared_ptr<UserStruct> dataStruct)
{
    data::User *targetUser = dataStruct->m_targetUser;

    for (size_t i = 0; i < targetUser->get_total_data_entries(); i++)
    {
        // Grab title for title.
        const char *target_title = data::get_title_info_by_id(targetUser->get_application_id_at(i))->get_title();

        // Update thread task.
        task->set_status(strings::get_by_name(strings::names::USER_OPTION_STATUS, 1), target_title);

        if (!fs::delete_save_data(*targetUser->get_save_info_at(i)))
        {
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::POP_MESSAGES_SAVE_CREATE, 2));
        }
    }
    task->finished();
}

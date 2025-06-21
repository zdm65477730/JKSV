#include "appstates/TitleOptionState.hpp"
#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TitleInfoState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "keyboard.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "system/system.hpp"
#include "ui/PopMessageManager.hpp"
#include <cstring>

namespace
{
    // Enum for menu.
    enum
    {
        INFORMATION,
        BLACKLIST,
        CHANGE_OUTPUT,
        FILE_MODE,
        DELETE_ALL_BACKUPS,
        RESET_SAVE_DATA,
        DELETE_SAVE_FROM_SYSTEM,
        EXTEND_CONTAINER,
        EXPORT_SVI
    };

    // Error string template thingies.
    static const char *ERROR_RESETTING_SAVE = "Error resetting save data: %s";
} // namespace

// Declarations. Definitions after class. Some of these are only here to be compatible with confirmations.
static void blacklist_title(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct);
static void change_output_path(data::TitleInfo *targetTitle);
static void delete_all_backups_for_title(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct);
static void reset_save_data(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct);
static void delete_save_data_from_system(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct);
static void extend_save_data(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct);
static void export_svi_file(data::TitleInfo *titleInfo);

TitleOptionState::TitleOptionState(data::User *user, data::TitleInfo *titleInfo, TitleSelectCommon *titleSelect)
    : m_user(user), m_titleInfo(titleInfo), m_titleSelect(titleSelect),
      m_dataStruct(std::make_shared<TitleOptionState::DataStruct>())
{
    // Create panel if needed.
    if (!sm_initialized)
    {
        // Allocate static members.
        sm_slidePanel = std::make_unique<ui::SlideOutPanel>(480, ui::SlideOutPanel::Side::Right);
        sm_titleOptionMenu = std::make_unique<ui::Menu>(8, 8, 460, 22, 720);

        // Populate menu.
        int stringIndex = 0;
        const char *currentString = nullptr;
        while ((currentString = strings::get_by_name(strings::names::TITLE_OPTIONS, stringIndex++)) != nullptr)
        {
            sm_titleOptionMenu->add_option(currentString);
        }

        // Only do this once.
        sm_initialized = true;
    }

    // Fill this out.
    m_dataStruct->m_user = m_user;
    m_dataStruct->m_titleInfo = m_titleInfo;
    m_dataStruct->m_spawningState = this;
    m_dataStruct->m_titleSelect = m_titleSelect;
}

void TitleOptionState::update(void)
{
    // This is kind of tricky to handle, because the blacklist function uses both.
    if (m_refreshRequired)
    {
        // Refresh the views.
        MainMenuState::refresh_view_states();
        m_refreshRequired = false;
        // Return so nothing else happens. Not sure I like this, but w/e.
        return;
    }
    if (m_exitRequired)
    {
        sm_slidePanel->close();
    }

    // Update panel and menu.
    sm_slidePanel->update(AppState::has_focus());
    sm_titleOptionMenu->update(AppState::has_focus());

    if (input::button_pressed(HidNpadButton_A))
    {
        switch (sm_titleOptionMenu->get_selected())
        {
            case INFORMATION:
            {
                auto titleInfoState = std::make_shared<TitleInfoState>(m_user, m_titleInfo);

                // Just push the state.
                StateManager::push_state(titleInfoState);
            }
            break;

            case BLACKLIST:
            {
                // Get the string.
                std::string confirmString = stringutil::get_formatted_string(
                    strings::get_by_name(strings::names::TITLE_OPTION_CONFIRMATIONS, 0),
                    m_titleInfo->get_title());

                // The actual state.
                auto confirm =
                    std::make_shared<ConfirmState<sys::Task, TaskState, TitleOptionState::DataStruct>>(confirmString,
                                                                                                       false,
                                                                                                       blacklist_title,
                                                                                                       m_dataStruct);

                // Push
                StateManager::push_state(confirm);
            }
            break;

            case CHANGE_OUTPUT:
            {
                change_output_path(m_titleInfo);
            }
            break;

            case FILE_MODE:
            {
            }
            break;

            case DELETE_ALL_BACKUPS:
            {
                // String
                std::string confirmString = stringutil::get_formatted_string(
                    strings::get_by_name(strings::names::TITLE_OPTION_CONFIRMATIONS, 1),
                    m_titleInfo->get_title());

                // State. This always requires holding because I hate people complaining to me about how it's my fault they don't read things first.
                auto confirm = std::make_shared<ConfirmState<sys::Task, TaskState, TitleOptionState::DataStruct>>(
                    confirmString,
                    true,
                    delete_all_backups_for_title,
                    m_dataStruct);

                StateManager::push_state(confirm);
            }
            break;

            case RESET_SAVE_DATA:
            {
                // Need to check this first. For safety.
                FsSaveDataInfo *saveInfo = m_user->get_save_info_by_id(m_titleInfo->get_application_id());
                if (fs::is_system_save_data(saveInfo) && !config::get_by_key(config::keys::ALLOW_WRITING_TO_SYSTEM))
                {
                    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                        strings::get_by_name(strings::names::TITLE_OPTION_POPS, 6));
                    return;
                }

                // String
                std::string confirmString = stringutil::get_formatted_string(
                    strings::get_by_name(strings::names::TITLE_OPTION_CONFIRMATIONS, 2),
                    m_titleInfo->get_title());

                auto confirm =
                    std::make_shared<ConfirmState<sys::Task, TaskState, TitleOptionState::DataStruct>>(confirmString,
                                                                                                       true,
                                                                                                       reset_save_data,
                                                                                                       m_dataStruct);

                StateManager::push_state(confirm);
            }
            break;

            case DELETE_SAVE_FROM_SYSTEM:
            {
                FsSaveDataInfo *saveInfo = m_user->get_save_info_by_id(m_titleInfo->get_application_id());
                if (fs::is_system_save_data(saveInfo) && !config::get_by_key(config::keys::ALLOW_WRITING_TO_SYSTEM))
                {
                    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                        strings::get_by_name(strings::names::TITLE_OPTION_POPS, 6));
                    return;
                }

                // String
                std::string confirmString = stringutil::get_formatted_string(
                    strings::get_by_name(strings::names::TITLE_OPTION_CONFIRMATIONS, 3),
                    m_user->get_nickname(),
                    m_titleInfo->get_title());

                // Confirmation.
                auto confirm = std::make_shared<ConfirmState<sys::Task, TaskState, TitleOptionState::DataStruct>>(
                    confirmString,
                    true,
                    delete_save_data_from_system,
                    m_dataStruct);

                StateManager::push_state(confirm);
            }
            break;

            case EXTEND_CONTAINER:
            {
                FsSaveDataInfo *saveInfo = m_user->get_save_info_by_id(m_titleInfo->get_application_id());
                if (fs::is_system_save_data(saveInfo) && !config::get_by_key(config::keys::ALLOW_WRITING_TO_SYSTEM))
                {
                    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                        strings::get_by_name(strings::names::TITLE_OPTION_POPS, 6));
                    return;
                }

                // State.
                StateManager::push_state(std::make_shared<TaskState>(extend_save_data, m_dataStruct));
            }
            break;

            case EXPORT_SVI:
            {
                // This type of save data can't have this exported anyway.
                FsSaveDataInfo *saveInfo = m_user->get_save_info_by_id(m_titleInfo->get_application_id());
                if (fs::is_system_save_data(saveInfo))
                {
                    return;
                }
                export_svi_file(m_titleInfo);
            }
            break;
        }
    }
    else if (input::button_pressed(HidNpadButton_B))
    {
        sm_slidePanel->close();
    }
    else if (sm_slidePanel->is_closed())
    {
        // Reset static members.
        sm_slidePanel->reset();
        sm_titleOptionMenu->set_selected(0);
        // Deactivate and allow state to be purged.
        AppState::deactivate();
    }
}

void TitleOptionState::render(void)
{
    sm_slidePanel->clear_target();
    sm_titleOptionMenu->render(sm_slidePanel->get_target(), AppState::has_focus());
    sm_slidePanel->render(NULL, AppState::has_focus());
}

void TitleOptionState::close_on_update(void)
{
    m_exitRequired = true;
}

void TitleOptionState::refresh_required(void)
{
    m_refreshRequired = true;
}

static void blacklist_title(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct)
{
    // Gonna need this a lot.
    uint64_t applicationID = dataStruct->m_titleInfo->get_application_id();

    // We're not gonna bother with a status for this. It'll flicker, but be barely noticeable.
    config::add_remove_blacklist(applicationID);

    // Now we need to remove it from all of the users. This doesn't just apply to the active one.
    data::UserList userList;
    data::get_users(userList);
    for (data::User *user : userList)
    {
        user->erase_save_info_by_id(applicationID);
    }

    // This will tell the main thread a refresh is required on the next update call.
    dataStruct->m_spawningState->refresh_required();
    dataStruct->m_spawningState->close_on_update();

    task->finished();
}

static void change_output_path(data::TitleInfo *targetTitle)
{
    // This is where we're writing the path.
    char pathBuffer[0x200] = {0};

    // Header string.
    std::string headerString =
        stringutil::get_formatted_string(strings::get_by_name(strings::names::KEYBOARD_STRINGS, 7),
                                         targetTitle->get_title());

    // Try to get input.
    if (!keyboard::get_input(SwkbdType_QWERTY, targetTitle->get_path_safe_title(), headerString, pathBuffer, 0x200))
    {
        return;
    }

    // Try to make sure it will work.
    if (!stringutil::sanitize_string_for_path(pathBuffer, pathBuffer, 0x200))
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_TITLE_OPTIONS, 0));
        return;
    }

    // Rename folder to match so there are no issues.
    fslib::Path oldPath = config::get_working_directory() / targetTitle->get_path_safe_title();
    fslib::Path newPath = config::get_working_directory() / pathBuffer;
    if (fslib::directory_exists(oldPath) && !fslib::rename_directory(oldPath, newPath))
    {
        // Bail if this fails, because something is really wrong.
        logger::log("Error setting new output path: %s", fslib::get_error_string());
        return;
    }

    // Add it to config and set target title to use it.
    targetTitle->set_path_safe_title(pathBuffer, std::strlen(pathBuffer));
    config::add_custom_path(targetTitle->get_application_id(), pathBuffer);

    // Pop so we know stuff happened.
    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                        strings::get_by_name(strings::names::POP_MESSAGES_TITLE_OPTIONS, 1),
                                        pathBuffer);
}

static void delete_all_backups_for_title(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct)
{
    // Get the path.
    fslib::Path titlePath = config::get_working_directory() / dataStruct->m_titleInfo->get_path_safe_title();

    // Set the status.
    task->set_status(strings::get_by_name(strings::names::TITLE_OPTION_STATUS, 0),
                     dataStruct->m_titleInfo->get_title());

    // Just call this and nuke the folder.
    if (!fslib::delete_directory_recursively(titlePath))
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::TITLE_OPTION_POPS, 1));
    }
    else
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::TITLE_OPTION_POPS, 0),
                                            dataStruct->m_titleInfo->get_title());
    }
    task->finished();
}

static void reset_save_data(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct)
{
    // To do: Make this not as hard to read.
    // Attempt to mount save.
    if (!fslib::open_save_data_with_save_info(
            fs::DEFAULT_SAVE_MOUNT,
            *dataStruct->m_user->get_save_info_by_id(dataStruct->m_titleInfo->get_application_id())))
    {
        logger::log(ERROR_RESETTING_SAVE, fslib::get_error_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::TITLE_OPTION_POPS, 2));
        task->finished();
        return;
    }

    // Wipe the root.
    if (!fslib::delete_directory_recursively(fs::DEFAULT_SAVE_ROOT))
    {
        fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);
        logger::log(ERROR_RESETTING_SAVE, fslib::get_error_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::TITLE_OPTION_POPS, 2));
        task->finished();
        return;
    }

    // Attempt commit.
    if (!fslib::commit_data_to_file_system(fs::DEFAULT_SAVE_MOUNT))
    {
        fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);
        logger::log(ERROR_RESETTING_SAVE, fslib::get_error_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::TITLE_OPTION_POPS, 2));
        task->finished();
        return;
    }

    // Should be good to go.
    fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);
    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                        strings::get_by_name(strings::names::TITLE_OPTION_POPS, 3));
    task->finished();
}

static void delete_save_data_from_system(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct)
{
    // Set the status in case this takes a little while.
    task->set_status(strings::get_by_name(strings::names::TITLE_OPTION_STATUS, 2),
                     dataStruct->m_user->get_nickname(),
                     dataStruct->m_titleInfo->get_title());

    // Grab the save data info pointer.
    uint64_t applicationID = dataStruct->m_titleInfo->get_application_id();
    FsSaveDataInfo *saveInfo = dataStruct->m_user->get_save_info_by_id(applicationID);
    if (saveInfo == nullptr)
    {
        logger::log("Error deleting save data for user. Target save data null?");
        task->finished();
        return;
    }

    if (!fs::delete_save_data(saveInfo))
    {
        // Just cleanup, I guess?
        task->finished();
        return;
    }

    // Erase the info from the user since it should have been deleted.
    dataStruct->m_user->erase_save_info_by_id(applicationID);

    // Refresh
    dataStruct->m_titleSelect->refresh();

    // Signal to close, because this save is no long valid.
    dataStruct->m_spawningState->close_on_update();

    // Done?
    task->finished();
}

static void extend_save_data(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct)
{
    // Grab this stuff to make stuff easier to read and type.
    data::TitleInfo *titleInfo = dataStruct->m_titleInfo;
    FsSaveDataInfo *saveInfo = dataStruct->m_user->get_save_info_by_id(titleInfo->get_application_id());

    if (!saveInfo)
    {
        logger::log("Error retrieving save data info to extend!");
        task->finished();
        return;
    }

    // Set the status.
    task->set_status(strings::get_by_name(strings::names::TITLE_OPTION_STATUS, 3),
                     dataStruct->m_user->get_nickname(),
                     dataStruct->m_titleInfo->get_title());

    // This is the header string.
    std::string_view keyboardString = strings::get_by_name(strings::names::KEYBOARD_STRINGS, 8);

    // Get how much to extend.
    char buffer[5] = {0};
    // No default. Maybe change this later?
    if (!keyboard::get_input(SwkbdType_NumPad, {}, keyboardString, buffer, 5))
    {
        task->finished();
        return;
    }

    // Convert input to number and multiply it by 1MB. To do: Check if this is valid before continuing?
    int64_t size = std::strtoll(buffer, NULL, 10) * 0x100000;

    // Grab the journal size.
    int64_t journalSize = titleInfo->get_journal_size(saveInfo->save_data_type);

    // To do: Check this and toast message.
    fs::extend_save_data(saveInfo, size, journalSize);

    task->finished();
}

static void export_svi_file(data::TitleInfo *titleInfo)
{
    // This is to allow the files to be create with a starting size. This cuts down on FS calls with fslib.
    constexpr size_t SIZE_SVI_FILE = sizeof(uint64_t) + sizeof(NsApplicationControlData);

    // Export path.
    fslib::Path sviPath = config::get_working_directory() / "svi" /
                          stringutil::get_formatted_string("%016llX.svi", titleInfo->get_application_id());

    // Check if it already exists.
    if (fslib::file_exists(sviPath))
    {
        logger::log("SVI for %016llX already exists!", titleInfo->get_application_id());
        // Just show this and bail.
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::TITLE_OPTION_POPS, 5));
        return;
    }

    // File
    fslib::File sviFile(sviPath, FsOpenMode_Create | FsOpenMode_Write, SIZE_SVI_FILE);
    if (!sviFile)
    {
        logger::log("Error exporting SVI file: %s", fslib::get_error_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::TITLE_OPTION_POPS, 5));
    }

    // Ok. Letsa go~
    // This is needed like this.
    uint64_t applicationID = titleInfo->get_application_id();

    // Write the stuff we need.
    sviFile.write(&applicationID, sizeof(uint64_t));
    sviFile.write(titleInfo->get_control_data(), sizeof(NsApplicationControlData));

    // Show this so we know things happened.jpg
    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                        strings::get_by_name(strings::names::TITLE_OPTION_POPS, 4));
}

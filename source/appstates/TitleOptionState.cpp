#include "appstates/TitleOptionState.hpp"
#include "appstates/ConfirmState.hpp"
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

// Struct to send data to functions that require confirmation.
typedef struct
{
        data::User *m_targetUser;
        data::TitleInfo *m_targetTitle;
} TargetStruct;

// Declarations. Definitions after class. Some of these are only here to be compatible with confirmations.
static void blacklist_title(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);
static void delete_all_backups_for_title(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);
static void reset_save_data(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);
static void delete_save_data_from_system(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);
static void change_output_path(data::TitleInfo *targetTitle);

TitleOptionState::TitleOptionState(data::User *user, data::TitleInfo *titleInfo)
    : m_targetUser(user), m_titleInfo(titleInfo)
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
}

void TitleOptionState::update(void)
{
    // Update panel and menu.
    sm_slidePanel->update(AppState::has_focus());
    sm_titleOptionMenu->update(AppState::has_focus());

    if (input::button_pressed(HidNpadButton_A))
    {
        switch (sm_titleOptionMenu->get_selected())
        {
            case INFORMATION:
            {
            }
            break;

            case BLACKLIST:
            {
                // Get the string.
                std::string confirmString = stringutil::get_formatted_string(
                    strings::get_by_name(strings::names::TITLE_OPTION_CONFIRMATIONS, 0),
                    m_titleInfo->get_title());

                // Data to send
                std::shared_ptr<TargetStruct> data = std::make_shared<TargetStruct>();
                data->m_targetTitle = m_titleInfo;

                // The actual state.
                std::shared_ptr<ConfirmState<sys::Task, TaskState, TargetStruct>> confirm =
                    std::make_shared<ConfirmState<sys::Task, TaskState, TargetStruct>>(confirmString,
                                                                                       false,
                                                                                       blacklist_title,
                                                                                       data);

                // Push
                JKSV::push_state(confirm);
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
            }
            break;

            case RESET_SAVE_DATA:
            {
            }
            break;

            case DELETE_SAVE_FROM_SYSTEM:
            {
            }
            break;

            case EXTEND_CONTAINER:
            {
            }
            break;

            case EXPORT_SVI:
            {
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
        AppState::deactivate();
    }
}

void TitleOptionState::render(void)
{
    sm_slidePanel->clear_target();
    sm_titleOptionMenu->render(sm_slidePanel->get(), AppState::has_focus());
    sm_slidePanel->render(NULL, AppState::has_focus());
}

static void blacklist_title(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // We're not gonna bother with a status for this. It'll flicker, but be barely noticeable.
    config::add_remove_blacklist(dataStruct->m_targetTitle->get_application_id());
    task->finished();
}

static void delete_all_backups_for_title(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // Get the path.
    fslib::Path titlePath = config::get_working_directory() / dataStruct->m_targetTitle->get_path_safe_title();

    // Set the status.
    task->set_status(strings::get_by_name(strings::names::TITLE_OPTION_STATUS, 0),
                     dataStruct->m_targetTitle->get_title());

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
                                            dataStruct->m_targetTitle->get_title());
    }
    task->finished();
}

static void reset_save_data(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // Attempt to mount save.
    if (!fslib::open_save_data_with_save_info(
            fs::DEFAULT_SAVE_MOUNT,
            *dataStruct->m_targetUser->get_save_info_by_id(dataStruct->m_targetTitle->get_application_id())))
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

static void delete_save_data_from_system(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // Set status. We're going to borrow this string from the other state's strings.
    task->set_status(strings::get_by_name(strings::names::USER_OPTION_STATUS, 1),
                     dataStruct->m_targetTitle->get_title());
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

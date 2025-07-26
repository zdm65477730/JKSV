#include "appstates/TitleOptionState.hpp"

#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TitleInfoState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "error.hpp"
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
    : m_user(user)
    , m_titleInfo(titleInfo)
    , m_titleSelect(titleSelect)
    , m_dataStruct(std::make_shared<TitleOptionState::DataStruct>())
{
    // Create panel if needed.
    if (!sm_initialized)
    {
        // Allocate static members.
        sm_slidePanel      = std::make_unique<ui::SlideOutPanel>(480, ui::SlideOutPanel::Side::Right);
        sm_titleOptionMenu = std::make_unique<ui::Menu>(8, 8, 460, 22, 720);

        // Populate menu.
        int stringIndex           = 0;
        const char *currentString = nullptr;
        while ((currentString = strings::get_by_name(strings::names::TITLEOPTION, stringIndex++)) != nullptr)
        {
            sm_titleOptionMenu->add_option(currentString);
        }

        // Only do this once.
        sm_initialized = true;
    }

    // Fill this out.
    m_dataStruct->user          = m_user;
    m_dataStruct->titleInfo     = m_titleInfo;
    m_dataStruct->spawningState = this;
    m_dataStruct->titleSelect   = m_titleSelect;
}

void TitleOptionState::update()
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
    if (m_exitRequired) { sm_slidePanel->close(); }

    // Update panel and menu.
    sm_slidePanel->update(BaseState::has_focus());
    sm_titleOptionMenu->update(BaseState::has_focus());

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
                std::string confirmString =
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::TITLEOPTION_CONFS, 0),
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
                std::string confirmString =
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::TITLEOPTION_CONFS, 1),
                                                     m_titleInfo->get_title());

                // State. This always requires holding because I hate people complaining to me about how it's my fault they
                // don't read things first.
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
                    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                                        strings::get_by_name(strings::names::TITLEOPTION_POPS, 6));
                    return;
                }

                // String
                std::string confirmString =
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::TITLEOPTION_CONFS, 2),
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
                    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                                        strings::get_by_name(strings::names::TITLEOPTION_POPS, 6));
                    return;
                }

                // String
                std::string confirmString =
                    stringutil::get_formatted_string(strings::get_by_name(strings::names::TITLEOPTION_CONFS, 3),
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
                    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                                        strings::get_by_name(strings::names::TITLEOPTION_POPS, 6));
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
                if (fs::is_system_save_data(saveInfo)) { return; }
                export_svi_file(m_titleInfo);
            }
            break;
        }
    }
    else if (input::button_pressed(HidNpadButton_B)) { sm_slidePanel->close(); }
    else if (sm_slidePanel->is_closed())
    {
        // Reset static members.
        sm_slidePanel->reset();
        sm_titleOptionMenu->set_selected(0);
        // Deactivate and allow state to be purged.
        BaseState::deactivate();
    }
}

void TitleOptionState::render()
{
    sm_slidePanel->clear_target();
    sm_titleOptionMenu->render(sm_slidePanel->get_target(), BaseState::has_focus());
    sm_slidePanel->render(NULL, BaseState::has_focus());
}

void TitleOptionState::close_on_update() { m_exitRequired = true; }

void TitleOptionState::refresh_required() { m_refreshRequired = true; }

static void blacklist_title(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct)
{
    if (error::is_null(task)) { return; }

    data::TitleInfo *titleInfo      = dataStruct->titleInfo;
    TitleOptionState *spawningState = dataStruct->spawningState;
    const uint64_t applicationID    = titleInfo->get_application_id();

    config::add_remove_blacklist(applicationID);

    data::UserList userList;
    data::get_users(userList);
    for (data::User *user : userList) { user->erase_save_info_by_id(applicationID); }

    // This will tell the main thread a refresh is required on the next update call.
    spawningState->refresh_required();
    spawningState->close_on_update();

    task->finished();
}

static void change_output_path(data::TitleInfo *targetTitle)
{
    static constexpr size_t SIZE_PATH_BUFFER = 0x200;
    const char *headerTemplate               = strings::get_by_name(strings::names::KEYBOARD, 7);
    const int popTicks                       = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popSuccess                   = strings::get_by_name(strings::names::TITLEOPTION_POPS, 1);
    const char *popFailure                   = strings::get_by_name(strings::names::TITLEOPTION_POPS, 0);
    const char *pathSafeTitle                = targetTitle->get_path_safe_title();

    const std::string headerString    = stringutil::get_formatted_string(headerTemplate, targetTitle->get_title());
    char pathBuffer[SIZE_PATH_BUFFER] = {0};
    const bool inputIsValid = keyboard::get_input(SwkbdType_QWERTY, pathSafeTitle, headerString, pathBuffer, SIZE_PATH_BUFFER);
    const bool sanitized    = inputIsValid && stringutil::sanitize_string_for_path(pathBuffer, pathBuffer, SIZE_PATH_BUFFER);
    if (!inputIsValid || !sanitized)
    {
        ui::PopMessageManager::push_message(popTicks, popFailure);
        return;
    }

    const fslib::Path workDir = config::get_working_directory();
    const fslib::Path oldPath{workDir / targetTitle->get_path_safe_title()};
    const fslib::Path newPath{workDir / pathBuffer};
    const bool dirExists    = fslib::directory_exists(oldPath);
    const bool renameFailed = dirExists && error::fslib(fslib::rename_directory(oldPath, newPath));
    if (dirExists && renameFailed)
    {
        ui::PopMessageManager::push_message(popTicks, popFailure);
        return;
    }

    targetTitle->set_path_safe_title(pathBuffer, std::strlen(pathBuffer));
    config::add_custom_path(targetTitle->get_application_id(), pathBuffer);

    const std::string popMessage = stringutil::get_formatted_string(popSuccess, pathBuffer);
    ui::PopMessageManager::push_message(popTicks, popMessage);
}

static void delete_all_backups_for_title(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct)
{
    if (error::is_null(task)) { return; }
    data::TitleInfo *titleInfo = dataStruct->titleInfo;

    const char *statusTemplate = strings::get_by_name(strings::names::TITLEOPTION_STATUS, 0);
    const int popTicks         = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popSuccess     = strings::get_by_name(strings::names::TITLEOPTION_POPS, 0);
    const char *popFailure     = strings::get_by_name(strings::names::TITLEOPTION_POPS, 1);

    {
        const std::string status = stringutil::get_formatted_string(statusTemplate, titleInfo->get_title());
        task->set_status(status);
    }

    const fslib::Path titlePath{config::get_working_directory() / titleInfo->get_path_safe_title()};
    const bool deleteFailed = error::fslib(fslib::delete_directory_recursively(titlePath));
    if (deleteFailed) { ui::PopMessageManager::push_message(popTicks, popFailure); }
    else { const std::string popMessage = stringutil::get_formatted_string(popSuccess, titleInfo->get_title()); }

    task->finished();
}

static void reset_save_data(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct)
{
    if (error::is_null(task)) { return; }

    data::User *user           = dataStruct->user;
    data::TitleInfo *titleInfo = dataStruct->titleInfo;

    const uint64_t applicationID   = titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(applicationID);
    const int popTicks             = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popFailed          = strings::get_by_name(strings::names::TITLEOPTION_POPS, 2);
    const char *popSucceeded       = strings::get_by_name(strings::names::TITLEOPTION_POPS, 3);

    const bool mountFailed = error::fslib(fslib::open_save_data_with_save_info(fs::DEFAULT_SAVE_MOUNT, *saveInfo));
    if (mountFailed)
    {
        ui::PopMessageManager::push_message(popTicks, popFailed);
        task->finished();
        return;
    }

    const bool wipeFailed   = error::fslib(fslib::delete_directory_recursively(fs::DEFAULT_SAVE_ROOT));
    const bool commitFailed = !wipeFailed && error::fslib(fslib::commit_data_to_file_system(fs::DEFAULT_SAVE_MOUNT));
    if (wipeFailed || commitFailed) { ui::PopMessageManager::push_message(popTicks, popFailed); }
    else { ui::PopMessageManager::push_message(popTicks, popSucceeded); }

    fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);
    task->finished();
}

static void delete_save_data_from_system(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct)
{
    data::User *user                = dataStruct->user;
    data::TitleInfo *titleInfo      = dataStruct->titleInfo;
    TitleSelectCommon *titleSelect  = dataStruct->titleSelect;
    TitleOptionState *spawningState = dataStruct->spawningState;

    const uint64_t applicationID   = titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(applicationID);
    const char *statusTemplate     = strings::get_by_name(strings::names::TITLEOPTION_STATUS, 2);
    if (error::is_null(task) || error::is_null(saveInfo)) { return; }

    {
        const char *nickname     = user->get_nickname();
        const char *title        = titleInfo->get_title();
        const std::string status = stringutil::get_formatted_string(statusTemplate, nickname, title);
        task->set_status(status);
    }

    const bool saveDeleted = fs::delete_save_data(saveInfo);
    if (!saveDeleted)
    {
        task->finished();
        return;
    }

    user->erase_save_info_by_id(applicationID);
    titleSelect->refresh();
    spawningState->close_on_update(); // Since the save was deleted, state is no longer valid.
    task->finished();
}

static void extend_save_data(sys::Task *task, std::shared_ptr<TitleOptionState::DataStruct> dataStruct)
{
    static constexpr size_t SIZE_EXTRA = sizeof(FsSaveDataExtraData);
    static constexpr size_t SIZE_MB    = 0x100000;

    data::User *user               = dataStruct->user;
    data::TitleInfo *titleInfo     = dataStruct->titleInfo;
    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(titleInfo->get_application_id());
    const char *statusTemplate     = strings::get_by_name(strings::names::TITLEOPTION_STATUS, 3);
    const char *keyboardHeader     = strings::get_by_name(strings::names::KEYBOARD, 8);
    const int popTicks             = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popSuccess         = strings::get_by_name(strings::names::TITLEOPTION_POPS, 10);
    const char *popFailed          = strings::get_by_name(strings::names::TITLEOPTION_POPS, 11);
    if (error::is_null(task) || error::is_null(saveInfo)) { return; }

    {
        const char *nickname     = user->get_nickname();
        const char *title        = titleInfo->get_title();
        const std::string status = stringutil::get_formatted_string(statusTemplate, nickname, title);
        task->set_status(status);
    }

    FsSaveDataExtraData extraData{};
    char buffer[5]        = {0};
    const bool extraError = error::libnx(fsReadSaveDataFileSystemExtraData(&extraData, SIZE_EXTRA, saveInfo->save_data_id));
    const std::string keyboardDefault = stringutil::get_formatted_string("%u", (extraData.data_size / SIZE_MB) + SIZE_MB);
    const bool validInput             = keyboard::get_input(SwkbdType_NumPad, keyboardDefault, keyboardHeader, buffer, 5);
    if (!validInput)
    {
        task->finished();
        return;
    }

    const uint8_t saveType  = saveInfo->save_data_type;
    const int64_t size      = std::strtoll(buffer, NULL, 10) * 0x100000;
    const int64_t journal   = extraError ? titleInfo->get_journal_size(saveType) : extraData.journal_size;
    const bool saveExtended = fs::extend_save_data(saveInfo, size, journal);
    if (saveExtended) { ui::PopMessageManager::push_message(popTicks, popSuccess); }
    else { ui::PopMessageManager::push_message(popTicks, popFailed); }

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
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                            strings::get_by_name(strings::names::TITLEOPTION_POPS, 5));
        return;
    }

    // File
    fslib::File sviFile(sviPath, FsOpenMode_Create | FsOpenMode_Write, SIZE_SVI_FILE);
    if (!sviFile)
    {
        logger::log("Error exporting SVI file: %s", fslib::error::get_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                            strings::get_by_name(strings::names::TITLEOPTION_POPS, 5));
    }

    // Ok. Letsa go~
    // This is needed like this.
    uint64_t applicationID = titleInfo->get_application_id();

    // Write the stuff we need.
    sviFile.write(&applicationID, sizeof(uint64_t));
    sviFile.write(titleInfo->get_control_data(), sizeof(NsApplicationControlData));

    // Show this so we know things happened.jpg
    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                        strings::get_by_name(strings::names::TITLEOPTION_POPS, 4));
}

#include "appstates/BackupMenuState.hpp"
#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/ProgressState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "keyboard.hpp"
#include "logger.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "system/system.hpp"
#include "ui/PopMessageManager.hpp"
#include "ui/TextScroll.hpp"
#include <cstring>

namespace
{
    /// @brief This is the length allotted for naming backups.
    constexpr size_t SIZE_BACKUP_NAME_LENGTH = 0x80;

    /// @brief This is just so there isn't random .zip comparisons everywhere.
    constexpr std::string_view STRING_ZIP_EXTENSION = ".zip";
} // namespace

// Declarations here. Definitions after class.
// Create new backup in targetPath
static void create_new_backup(sys::ProgressTask *task,
                              data::User *user,
                              data::TitleInfo *titleInfo,
                              fslib::Path targetPath,
                              BackupMenuState *spawningState);
// Overwrites and existing backup.
static void overwrite_backup(sys::ProgressTask *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct);
// Restores a backup and requires confirmation to do so. Takes a shared_ptr to a TargetStruct.
static void restore_backup(sys::ProgressTask *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct);
// Deletes a backup and requires confirmation to do so. Takes a shared_ptr to a TargetStruct.
static void delete_backup(sys::Task *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct);

BackupMenuState::BackupMenuState(data::User *user, data::TitleInfo *titleInfo, FsSaveDataType saveType)
    : m_user(user), m_titleInfo(titleInfo), m_saveType(saveType),
      m_directoryPath(config::get_working_directory() / m_titleInfo->get_path_safe_title()),
      m_dataStruct(std::make_shared<BackupMenuState::DataStruct>())
{
    if (!sm_isInitialized)
    {
        sm_panelWidth = sdl::text::get_width(22, strings::get_by_name(strings::names::CONTROL_GUIDES, 2)) + 64;
        // To do: Give classes an alternate so they don't have to be constructed.
        sm_backupMenu = std::make_shared<ui::Menu>(8, 8, sm_panelWidth - 14, 24, 600);
        sm_slidePanel = std::make_unique<ui::SlideOutPanel>(sm_panelWidth, ui::SlideOutPanel::Side::Right);
        sm_menuRenderTarget =
            sdl::TextureManager::create_load_texture("backupMenuTarget",
                                                     sm_panelWidth,
                                                     600,
                                                     SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
        sm_isInitialized = true;
    }

    // Fill this out. Target path is not set here.
    m_dataStruct->m_user = m_user;
    m_dataStruct->m_titleInfo = m_titleInfo;
    m_dataStruct->m_journalSize = m_titleInfo->get_journal_size(m_saveType);
    m_dataStruct->m_spawningState = this;

    // String for the top of the panel.
    std::string panelString =
        stringutil::get_formatted_string("`%s` - %s", m_user->get_nickname(), m_titleInfo->get_title());

    // This needs sm_panelWidth or it'd be in the initializer list.
    m_titleScroll.create(panelString, 22, sm_panelWidth, 8, true, colors::WHITE);

    // This is a quick check to make sure the save has something in it before creating empty backups.
    fslib::Directory saveCheck(fs::DEFAULT_SAVE_ROOT);
    m_saveHasData = saveCheck.get_count() > 0;

    // This just fills out the menu.
    BackupMenuState::refresh();
}

BackupMenuState::~BackupMenuState()
{
    sm_slidePanel->clear_elements();
}

void BackupMenuState::update(void)
{
    bool hasFocus = AppState::has_focus();

    if (input::button_pressed(HidNpadButton_A) && sm_backupMenu->get_selected() == 0 && m_saveHasData)
    {
        // get name for backup.
        char backupName[SIZE_BACKUP_NAME_LENGTH + 1] = {0};

        // Set backup to default.
        std::snprintf(backupName,
                      SIZE_BACKUP_NAME_LENGTH,
                      "%s - %s",
                      m_user->get_path_safe_nickname(),
                      stringutil::get_date_string().c_str());

        if (!input::button_held(HidNpadButton_ZR) &&
            !keyboard::get_input(SwkbdType_QWERTY,
                                 backupName,
                                 strings::get_by_name(strings::names::KEYBOARD_STRINGS, 0),
                                 backupName,
                                 SIZE_BACKUP_NAME_LENGTH))
        {
            return;
        }
        // To do: This isn't a good way to check for this... Check to make sure zip has zip extension.
        if (config::get_by_key(config::keys::EXPORT_TO_ZIP) && std::strstr(backupName, ".zip") == NULL)
        {
            // I guess this is a safer way to accomplish this?
            std::strncat(backupName, ".zip", SIZE_BACKUP_NAME_LENGTH);
        }
        else if (!config::get_by_key(config::keys::EXPORT_TO_ZIP) && !std::strstr(backupName, ".zip") &&
                 !fslib::directory_exists(m_directoryPath / backupName) &&
                 !fslib::create_directory(m_directoryPath / backupName))
        {
            return;
        }

        // This is the path to write the backup to.
        fslib::Path targetPath = m_directoryPath / backupName;

        // Create the state.
        auto createNewBackup =
            std::make_shared<ProgressState>(create_new_backup, m_user, m_titleInfo, targetPath, this);

        // Push the task.
        StateManager::push_state(createNewBackup);
    }
    else if (input::button_pressed(HidNpadButton_A) && sm_backupMenu->get_selected() == 0 && !m_saveHasData)
    {
        // This just makes the little no save data found pop up.
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 0));
    }
    else if (input::button_pressed(HidNpadButton_A) && m_saveHasData && sm_backupMenu->get_selected() > 0)
    {
        int selected = sm_backupMenu->get_selected() - 1;

        std::string queryString =
            stringutil::get_formatted_string(strings::get_by_name(strings::names::BACKUPMENU_CONFIRMATIONS, 0),
                                             m_directoryListing[selected]);

        // Set the target path quick.
        m_dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];

        auto confirm = std::make_shared<ConfirmState<sys::ProgressTask, ProgressState, BackupMenuState::DataStruct>>(
            queryString,
            config::get_by_key(config::keys::HOLD_FOR_OVERWRITE),
            overwrite_backup,
            m_dataStruct);

        StateManager::push_state(confirm);
    }
    else if (input::button_pressed(HidNpadButton_A) && !m_saveHasData && sm_backupMenu->get_selected() > 0)
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 0));
    }
    else if (input::button_pressed(HidNpadButton_Y) && sm_backupMenu->get_selected() > 0 &&
             (m_saveType != FsSaveDataType_System || config::get_by_key(config::keys::ALLOW_WRITING_TO_SYSTEM)))
    {
        // Need to account for new at the top.
        int selected = sm_backupMenu->get_selected() - 1;

        // Gonna need to test this quick.
        fslib::Path targetPath = m_directoryPath / m_directoryListing[selected];

        // This is a quick check to avoid restoring blanks.
        if (fslib::directory_exists(targetPath) && !fs::directory_has_contents(targetPath))
        {
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 1));
            return;
        }
        else if (fslib::file_exists(targetPath) && std::strcmp("zip", targetPath.get_extension()) == 0 &&
                 !fs::zip_has_contents(targetPath))
        {
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 1));
            return;
        }

        // Set target path
        m_dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];

        std::string queryString =
            stringutil::get_formatted_string(strings::get_by_name(strings::names::BACKUPMENU_CONFIRMATIONS, 1),
                                             m_directoryListing[selected]);

        auto confirm = std::make_shared<ConfirmState<sys::ProgressTask, ProgressState, BackupMenuState::DataStruct>>(
            queryString,
            config::get_by_key(config::keys::HOLD_FOR_RESTORATION),
            restore_backup,
            m_dataStruct);

        StateManager::push_state(confirm);
    }
    else if (input::button_pressed(HidNpadButton_X) && sm_backupMenu->get_selected() > 0)
    {
        // Selected needs to be offset by one to account for New
        int selected = sm_backupMenu->get_selected() - 1;

        // Set path quick.
        m_dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];

        // get the string.
        std::string queryString =
            stringutil::get_formatted_string(strings::get_by_name(strings::names::BACKUPMENU_CONFIRMATIONS, 2),
                                             m_directoryListing[selected]);

        auto confirm = std::make_shared<ConfirmState<sys::Task, TaskState, BackupMenuState::DataStruct>>(
            queryString,
            config::get_by_key(config::keys::HOLD_FOR_DELETION),
            delete_backup,
            m_dataStruct);

        // Create/push new state.
        StateManager::push_state(confirm);
    }
    else if (input::button_pressed(HidNpadButton_B))
    {
        fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);
        sm_slidePanel->close();
    }
    else if (sm_slidePanel->is_closed())
    {
        sm_slidePanel->reset();
        AppState::deactivate();
    }

    // Update title scrolling.
    m_titleScroll.update(hasFocus);
    // Update panel.
    sm_slidePanel->update(hasFocus);
    // This state bypasses the Slideout panel's normal behavior because it kind of has to.
    sm_backupMenu->update(hasFocus);
}

void BackupMenuState::render(void)
{
    // Save this locally.
    bool hasFocus = AppState::has_focus();

    // Clear panel target.
    sm_slidePanel->clear_target();

    // Grab the render target.
    SDL_Texture *slideTarget = sm_slidePanel->get_target();

    // Start with [Name] - [Title]
    m_titleScroll.render(slideTarget, hasFocus);

    sdl::render_line(slideTarget, 10, 42, sm_panelWidth - 10, 42, colors::WHITE);
    sdl::render_line(slideTarget, 10, 648, sm_panelWidth - 10, 648, colors::WHITE);
    sdl::text::render(slideTarget,
                      32,
                      673,
                      22,
                      sdl::text::NO_TEXT_WRAP,
                      colors::WHITE,
                      strings::get_by_name(strings::names::CONTROL_GUIDES, 2));

    // Clear menu target.
    sm_menuRenderTarget->clear(colors::TRANSPARENT);
    // render menu to it.
    sm_backupMenu->render(sm_menuRenderTarget->get(), hasFocus);
    // render it to panel target.
    sm_menuRenderTarget->render(slideTarget, 0, 43);

    sm_slidePanel->render(NULL, hasFocus);
}

void BackupMenuState::refresh(void)
{
    m_directoryListing.open(m_directoryPath);
    if (!m_directoryListing)
    {
        return;
    }

    sm_backupMenu->reset();
    sm_backupMenu->add_option(strings::get_by_name(strings::names::BACKUP_MENU, 0));
    for (int64_t i = 0; i < m_directoryListing.get_count(); i++)
    {
        sm_backupMenu->add_option(m_directoryListing[i]);
    }
}

void BackupMenuState::save_data_written(void)
{
    if (!m_saveHasData)
    {
        m_saveHasData = true;
    }
}

// This is the function to create new backups.
static void create_new_backup(sys::ProgressTask *task,
                              data::User *user,
                              data::TitleInfo *titleInfo,
                              fslib::Path targetPath,
                              BackupMenuState *spawningState)
{
    // SaveMeta
    FsSaveDataInfo *saveInfo = user->get_save_info_by_id(titleInfo->get_application_id());
    if (!saveInfo)
    {
        logger::log("Error retrieving save data information for %016lX.", titleInfo->get_application_id());
        task->finished();
        return;
    }

    // I got tired of typing out the cast.
    fs::SaveMetaData saveMeta;
    bool hasMeta = fs::fill_save_meta_data(saveInfo, saveMeta);

    // This extension search is lazy and needs to be revised.
    if (config::get_by_key(config::keys::EXPORT_TO_ZIP) || std::strstr(targetPath.c_string(), "zip"))
    {
        zipFile newBackup = zipOpen64(targetPath.c_string(), APPEND_STATUS_CREATE);
        if (!newBackup)
        {
            // To do: Pop up.
            logger::log("Error opening zip for backup.");
            task->finished();
            return;
        }

        if (hasMeta)
        {
            // Data for save meta.
            zip_fileinfo saveMetaInfo;
            fs::create_zip_fileinfo(saveMetaInfo);

            // Write meta to zip.
            int zipError = zipOpenNewFileInZip64(newBackup,
                                                 fs::NAME_SAVE_META.data(),
                                                 &saveMetaInfo,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 Z_DEFLATED,
                                                 config::get_by_key(config::keys::ZIP_COMPRESSION_LEVEL),
                                                 0);
            if (zipError == ZIP_OK)
            {
                zipWriteInFileInZip(newBackup, &saveMeta, sizeof(fs::SaveMetaData));
                zipCloseFileInZip(newBackup);
            }
        }
        fs::copy_directory_to_zip(fs::DEFAULT_SAVE_ROOT, newBackup, task);
        zipClose(newBackup, NULL);
    }
    else
    {
        {
            fslib::Path saveMetaPath = targetPath / fs::NAME_SAVE_META;
            fslib::File saveMetaOut(saveMetaPath, FsOpenMode_Create | FsOpenMode_Write, sizeof(fs::SaveMetaData));
            if (saveMetaOut && hasMeta)
            {
                saveMetaOut.write(&saveMeta, sizeof(fs::SaveMetaData));
            }
        }
        fs::copy_directory(fs::DEFAULT_SAVE_ROOT, targetPath, 0, {}, task);
    }

    // This should actually fatal somehow?
    if (!fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT))
    {
        logger::log("Error closing save data: %s.", fslib::get_error_string());
    }

    // Refresh.
    spawningState->refresh();

    task->finished();
}

static void overwrite_backup(sys::ProgressTask *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct)
{
    // I hate typing this stuff over and over.
    static const char *STRING_ERROR_PREFIX = "Error overwriting backup: %s";

    // Might need this later.
    FsSaveDataInfo *saveInfo = dataStruct->m_user->get_save_info_by_id(dataStruct->m_titleInfo->get_application_id());

    // Wew this is a fun one to read, but it takes care of everything in one go.
    if ((fslib::directory_exists(dataStruct->m_targetPath) &&
         !fslib::delete_directory_recursively(dataStruct->m_targetPath)) ||
        (fslib::file_exists(dataStruct->m_targetPath) && !fslib::delete_file(dataStruct->m_targetPath)))
    {
        // Try to close this quick.
        fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);
        logger::log(STRING_ERROR_PREFIX, fslib::get_error_string());
        task->finished();
        return;
    }

    // Gonna need a new save meta.
    fs::SaveMetaData meta;
    bool hasMeta = fs::fill_save_meta_data(saveInfo, meta);


    if (std::strstr(STRING_ZIP_EXTENSION.data(), dataStruct->m_targetPath.c_string()))
    {
        zipFile backupZip = zipOpen64(dataStruct->m_targetPath.c_string(), APPEND_STATUS_CREATE);
        if (!backupZip)
        {
            logger::log("Error overwriting backup: Couldn't create new zip!");
            task->finished();
            return;
        }


        // Need the zip info for the meta.
        if (hasMeta)
        {
            zip_fileinfo saveMetaInfo;
            fs::create_zip_fileinfo(saveMetaInfo);

            int zipError = zipOpenNewFileInZip64(backupZip,
                                                 fs::NAME_SAVE_META.data(),
                                                 &saveMetaInfo,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 Z_DEFLATED,
                                                 config::get_by_key(config::keys::ZIP_COMPRESSION_LEVEL),
                                                 0);
            if (zipError == ZIP_OK)
            {
                zipWriteInFileInZip(backupZip, &meta, sizeof(fs::SaveMetaData));
                zipCloseFileInZip(backupZip);
            }
        }

        fs::copy_directory_to_zip(fs::DEFAULT_SAVE_ROOT, backupZip, task);
        zipClose(backupZip, NULL);
    } // I hope this check works for making sure this is a folder
    else if (dataStruct->m_targetPath.get_extension() == nullptr && fslib::create_directory(dataStruct->m_targetPath))
    {
        // Write this quick.
        {
            fslib::Path metaPath = dataStruct->m_targetPath / fs::NAME_SAVE_META;
            fslib::File metaFile(metaPath, FsOpenMode_Create | FsOpenMode_Write, sizeof(fs::SaveMetaData));
            if (metaFile && hasMeta)
            {
                metaFile.write(&meta, sizeof(fs::SaveMetaData));
            }
        }

        fs::copy_directory(fs::DEFAULT_SAVE_ROOT, dataStruct->m_targetPath, 0, {}, task);
    }
    task->finished();
}

static void restore_backup(sys::ProgressTask *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct)
{
    // Going to need this later.
    FsSaveDataInfo *saveInfo = dataStruct->m_user->get_save_info_by_id(dataStruct->m_titleInfo->get_application_id());
    if (!saveInfo)
    {
        // To do: Log this.
        task->finished();
        return;
    }

    // Wipe the save root first. Forgot to commit the changes before. Oops.
    if (!fslib::delete_directory_recursively(fs::DEFAULT_SAVE_ROOT) ||
        !fslib::commit_data_to_file_system(fs::DEFAULT_SAVE_MOUNT))
    {
        logger::log("Error restoring save: %s", fslib::get_error_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 2));
        task->finished();
        return;
    }

    if (fslib::directory_exists(dataStruct->m_targetPath))
    {
        {
            // Process the save meta if it's there.
            fs::SaveMetaData meta = {0};
            fslib::Path saveMetaPath = dataStruct->m_targetPath / fs::NAME_SAVE_META;
            fslib::File saveMetaFile(saveMetaPath, FsOpenMode_Read);
            if (saveMetaFile && saveMetaFile.read(&meta, sizeof(fs::SaveMetaData)) == sizeof(fs::SaveMetaData))
            {
                // Set this so at least the user knows something is going on. Extending saves can take a decent chunk of time.
                task->set_status(strings::get_by_name(strings::names::BACKUPMENU_STATUS, 0));
                fs::process_save_meta_data(saveInfo, meta);
            }
        }

        fs::copy_directory(dataStruct->m_targetPath,
                           fs::DEFAULT_SAVE_ROOT,
                           dataStruct->m_journalSize,
                           fs::DEFAULT_SAVE_MOUNT,
                           task);
    }
    else if (std::strstr(dataStruct->m_targetPath.c_string(), ".zip") != NULL)
    {
        unzFile targetZip = unzOpen64(dataStruct->m_targetPath.c_string());
        if (!targetZip)
        {
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 3));
            logger::log("Error opening zip for reading.");
            task->finished();
            return;
        }

        {
            // I'm not sure if this is risky or not. Guess we'll find out...
            fs::SaveMetaData meta = {0};
            // The locate_file_in_zip should pinpoint the meta file.
            if (fs::locate_file_in_zip(targetZip, fs::NAME_SAVE_META) && unzOpenCurrentFile(targetZip) == UNZ_OK &&
                unzReadCurrentFile(targetZip, &meta, sizeof(fs::SaveMetaData)) == sizeof(fs::SaveMetaData))
            {
                task->set_status(strings::get_by_name(strings::names::BACKUPMENU_STATUS, 0));
                fs::process_save_meta_data(saveInfo, meta);
            }
        }

        fs::copy_zip_to_directory(targetZip,
                                  fs::DEFAULT_SAVE_ROOT,
                                  dataStruct->m_journalSize,
                                  fs::DEFAULT_SAVE_MOUNT,
                                  task);
        unzClose(targetZip);
    }
    else
    {
        fs::copy_file(dataStruct->m_targetPath,
                      fs::DEFAULT_SAVE_ROOT,
                      dataStruct->m_journalSize,
                      fs::DEFAULT_SAVE_MOUNT,
                      task);
    }

    // Update this just in case.
    dataStruct->m_spawningState->save_data_written();

    task->finished();
}

static void delete_backup(sys::Task *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct)
{
    if (task)
    {
        task->set_status(strings::get_by_name(strings::names::DELETING_FILES, 0), dataStruct->m_targetPath.c_string());
    }

    if (fslib::directory_exists(dataStruct->m_targetPath) &&
        !fslib::delete_directory_recursively(dataStruct->m_targetPath))
    {
        logger::log("Error deleting folder backup: %s", fslib::get_error_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 4));
    }
    else if (fslib::file_exists(dataStruct->m_targetPath) && !fslib::delete_file(dataStruct->m_targetPath))
    {
        logger::log("Error deleting backup: %s", fslib::get_error_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 4));
    }
    dataStruct->m_spawningState->refresh();
    task->finished();
}

#include "appstates/BackupMenuState.hpp"
#include "JKSV.hpp"
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

// This struct is used to pass data to Restore, Delete, and upload.
struct TargetStruct
{
        // Path of target.
        fslib::Path m_targetPath;
        // Journal size if commit is needed.
        uint64_t m_journalSize;
        // User
        data::User *m_user;
        // Title info
        data::TitleInfo *m_titleInfo;
        // Spawning state so refresh can be called.
        BackupMenuState *m_spawningState = nullptr;
};

// Declarations here. Definitions after class.
// Create new backup in targetPath
static void create_new_backup(sys::ProgressTask *task,
                              data::User *user,
                              data::TitleInfo *titleInfo,
                              fslib::Path targetPath,
                              BackupMenuState *spawningState);
// Overwrites and existing backup.
static void overwrite_backup(sys::ProgressTask *task, std::shared_ptr<TargetStruct> dataStruct);
// Restores a backup and requires confirmation to do so. Takes a shared_ptr to a TargetStruct.
static void restore_backup(sys::ProgressTask *task, std::shared_ptr<TargetStruct> dataStruct);
// Deletes a backup and requires confirmation to do so. Takes a shared_ptr to a TargetStruct.
static void delete_backup(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);

BackupMenuState::BackupMenuState(data::User *user, data::TitleInfo *titleInfo, FsSaveDataType saveType)
    : m_user(user), m_titleInfo(titleInfo), m_saveType(saveType),
      m_directoryPath(config::get_working_directory() / m_titleInfo->get_path_safe_title())
{
    if (!sm_isInitialized)
    {
        sm_panelWidth = sdl::text::getWidth(22, strings::get_by_name(strings::names::CONTROL_GUIDES, 2)) + 64;
        // To do: Give classes an alternate so they don't have to be constructed.
        sm_backupMenu = std::make_shared<ui::Menu>(8, 8, sm_panelWidth - 14, 24, 600);
        sm_slidePanel = std::make_unique<ui::SlideOutPanel>(sm_panelWidth, ui::SlideOutPanel::Side::Right);
        sm_menuRenderTarget =
            sdl::TextureManager::createLoadTexture("backupMenuTarget",
                                                   sm_panelWidth,
                                                   600,
                                                   SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
        sm_isInitialized = true;
    }

    // String for the top of the panel.
    std::string panelString =
        stringutil::get_formatted_string("`%s` - %s", m_user->get_nickname(), m_titleInfo->get_title());

    // This needs sm_panelWidth or it'd be in the initializer list.
    sm_slidePanel->push_new_element(std::make_shared<ui::TextScroll>(panelString, 22, sm_panelWidth, 8, colors::WHITE));


    fslib::Directory saveCheck(fs::DEFAULT_SAVE_PATH);
    m_saveHasData = saveCheck.getCount() > 0;

    BackupMenuState::refresh();
}

BackupMenuState::~BackupMenuState()
{
    sm_slidePanel->clear_elements();
}

void BackupMenuState::update(void)
{
    if (input::button_pressed(HidNpadButton_A) && sm_backupMenu->get_selected() == 0 && m_saveHasData)
    {
        // get name for backup.
        char backupName[0x81] = {0};

        // Set backup to default.
        std::snprintf(backupName,
                      0x80,
                      "%s - %s",
                      m_user->get_path_safe_nickname(),
                      stringutil::get_date_string().c_str());

        if (!input::button_held(HidNpadButton_ZR) &&
            !keyboard::get_input(SwkbdType_QWERTY,
                                 backupName,
                                 strings::get_by_name(strings::names::KEYBOARD_STRINGS, 0),
                                 backupName,
                                 0x80))
        {
            return;
        }
        // To do: This isn't a good way to check for this... Check to make sure zip has zip extension.
        if (config::get_by_key(config::keys::EXPORT_TO_ZIP) && std::strstr(backupName, ".zip") == NULL)
        {
            // To do: I should check this.
            std::strcat(backupName, ".zip");
        }
        else if (!config::get_by_key(config::keys::EXPORT_TO_ZIP) && !std::strstr(backupName, ".zip") &&
                 !fslib::directoryExists(m_directoryPath / backupName) &&
                 !fslib::createDirectory(m_directoryPath / backupName))
        {
            return;
        }
        // Push the task.
        JKSV::push_state(std::make_shared<ProgressState>(create_new_backup,
                                                         m_user,
                                                         m_titleInfo,
                                                         m_directoryPath / backupName,
                                                         this));
    }
    else if (input::button_pressed(HidNpadButton_A) && sm_backupMenu->get_selected() == 0 && !m_saveHasData)
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 0));
    }
    else if (input::button_pressed(HidNpadButton_A) && m_saveHasData && sm_backupMenu->get_selected() > 0)
    {
        int selected = sm_backupMenu->get_selected() - 1;

        std::string queryString =
            stringutil::get_formatted_string(strings::get_by_name(strings::names::BACKUPMENU_CONFIRMATIONS, 0),
                                             m_directoryListing[selected]);

        std::shared_ptr<TargetStruct> dataStruct(new TargetStruct);
        dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];

        JKSV::push_state(std::make_shared<ConfirmState<sys::ProgressTask, ProgressState, TargetStruct>>(
            queryString,
            config::get_by_key(config::keys::HOLD_FOR_OVERWRITE),
            overwrite_backup,
            dataStruct));
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
        if (fslib::directoryExists(targetPath) && !fs::directory_has_contents(targetPath))
        {
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 1));
            return;
        }
        else if (fslib::fileExists(targetPath) && std::strcmp("zip", targetPath.getExtension()) == 0 &&
                 !fs::zip_has_contents(targetPath))
        {
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 1));
            return;
        }

        std::shared_ptr<TargetStruct> dataStruct(new TargetStruct);
        dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];
        dataStruct->m_journalSize = m_titleInfo->get_journal_size(m_saveType);
        dataStruct->m_spawningState = this;

        std::string queryString =
            stringutil::get_formatted_string(strings::get_by_name(strings::names::BACKUPMENU_CONFIRMATIONS, 1),
                                             m_directoryListing[selected]);

        JKSV::push_state(std::make_shared<ConfirmState<sys::ProgressTask, ProgressState, TargetStruct>>(
            queryString,
            config::get_by_key(config::keys::HOLD_FOR_RESTORATION),
            restore_backup,
            dataStruct));
    }
    else if (input::button_pressed(HidNpadButton_X) && sm_backupMenu->get_selected() > 0)
    {
        // Selected needs to be offset by one to account for New
        int selected = sm_backupMenu->get_selected() - 1;

        // Create struct to pass.
        std::shared_ptr<TargetStruct> dataStruct(new TargetStruct);
        dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];
        dataStruct->m_spawningState = this;

        // get the string.
        std::string queryString =
            stringutil::get_formatted_string(strings::get_by_name(strings::names::BACKUPMENU_CONFIRMATIONS, 2),
                                             m_directoryListing[selected]);

        // Create/push new state.
        JKSV::push_state(std::make_shared<ConfirmState<sys::Task, TaskState, TargetStruct>>(
            queryString,
            config::get_by_key(config::keys::HOLD_FOR_DELETION),
            delete_backup,
            dataStruct));
    }
    else if (input::button_pressed(HidNpadButton_B))
    {
        fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);
        sm_slidePanel->close();
    }
    else if (sm_slidePanel->is_closed())
    {
        sm_slidePanel->reset();
        AppState::deactivate();
    }
    // Update panel.
    sm_slidePanel->update(AppState::has_focus());
    // This state bypasses the Slideout panel's normal behavior because it kind of has to.
    sm_backupMenu->update(AppState::has_focus());
}

void BackupMenuState::render(void)
{
    // Clear panel target.
    sm_slidePanel->clear_target();

    // Grab the render target.
    SDL_Texture *slideTarget = sm_slidePanel->get();

    sdl::renderLine(slideTarget, 10, 42, sm_panelWidth - 10, 42, colors::WHITE);
    sdl::renderLine(slideTarget, 10, 648, sm_panelWidth - 10, 648, colors::WHITE);
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
    sm_backupMenu->render(sm_menuRenderTarget->get(), AppState::has_focus());
    // render it to panel target.
    sm_menuRenderTarget->render(sm_slidePanel->get(), 0, 43);

    sm_slidePanel->render(NULL, AppState::has_focus());
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
    for (int64_t i = 0; i < m_directoryListing.getCount(); i++)
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

    // I got tired of typing out the cast.
    fs::SaveMetaData saveMeta = {.m_magic = fs::SAVE_META_MAGIC,
                                 .m_applicationID = titleInfo->get_application_id(),
                                 .m_saveType = saveInfo->save_data_type,
                                 .m_saveRank = saveInfo->save_data_rank,
                                 .m_saveSpaceID = saveInfo->save_data_space_id,
                                 .m_saveDataSize = titleInfo->get_save_data_size(saveInfo->save_data_type),
                                 .m_saveDataSizeMax = titleInfo->get_save_data_size_max(saveInfo->save_data_type),
                                 .m_journalSize = titleInfo->get_journal_size(saveInfo->save_data_type),
                                 .m_journalSizeMax = titleInfo->get_journal_size_max(saveInfo->save_data_type),
                                 .m_totalSaveSize = fs::get_directory_total_size(fs::DEFAULT_SAVE_PATH)};

    // This extension search is lazy and needs to be revised.
    if (config::get_by_key(config::keys::EXPORT_TO_ZIP) || std::strcmp("zip", targetPath.getExtension()) == 0)
    {
        zipFile newBackup = zipOpen64(targetPath.cString(), APPEND_STATUS_CREATE);
        if (!newBackup)
        {
            // To do: Pop up.
            logger::log("Error opening zip for backup.");
            return;
        }

        // Write meta to zip.
        int zipError = zipOpenNewFileInZip64(newBackup,
                                             ".jksv_save_meta.bin",
                                             NULL,
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

        fs::copy_directory_to_zip(fs::DEFAULT_SAVE_PATH, newBackup, task);
        zipClose(newBackup, NULL);
    }
    else
    {
        {
            // Write save meta quick.
            fslib::Path saveMetaPath = targetPath / ".jksv_save_meta.bin";
            fslib::File saveMetaOut(targetPath, FsOpenMode_Create | FsOpenMode_Write, sizeof(fs::SaveMetaData));
            if (saveMetaOut)
            {
                saveMetaOut.write(&saveMeta, sizeof(fs::SaveMetaData));
            }
        }

        fs::copy_directory(fs::DEFAULT_SAVE_PATH, targetPath, 0, {}, task);
    }
    spawningState->refresh();
    task->finished();
}

static void overwrite_backup(sys::ProgressTask *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // DirectoryExists can also be used to check if the target is a directory.
    if (fslib::directoryExists(dataStruct->m_targetPath) &&
        !fslib::deleteDirectoryRecursively(dataStruct->m_targetPath))
    {
        logger::log("Error overwriting backup: %s", fslib::getErrorString());
        task->finished();
        return;
    } // This has an added check for the zip extension so it can't try to overwrite files that aren't supposed to be zip.
    else if (fslib::fileExists(dataStruct->m_targetPath) &&
             std::strcmp("zip", dataStruct->m_targetPath.getExtension()) == 0 &&
             !fslib::deleteFile(dataStruct->m_targetPath))
    {
        logger::log("Error overwriting backup: %s", fslib::getErrorString());
        task->finished();
        return;
    }

    if (std::strcmp("zip", dataStruct->m_targetPath.getExtension()))
    {
        zipFile backupZip = zipOpen64(dataStruct->m_targetPath.cString(), APPEND_STATUS_CREATE);
        fs::copy_directory_to_zip(fs::DEFAULT_SAVE_PATH, backupZip, task);
        zipClose(backupZip, NULL);
    } // I hope this check works for making sure this is a folder
    else if (dataStruct->m_targetPath.getExtension() == nullptr && fslib::createDirectory(dataStruct->m_targetPath))
    {
        fs::copy_directory(fs::DEFAULT_SAVE_PATH, dataStruct->m_targetPath, 0, {}, task);
    }
    task->finished();
}

static void restore_backup(sys::ProgressTask *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // Wipe the save root first.
    if (!fslib::deleteDirectoryRecursively(fs::DEFAULT_SAVE_PATH))
    {
        logger::log("Error restoring save: %s", fslib::getErrorString());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 2));
        task->finished();
        return;
    }

    if (fslib::directoryExists(dataStruct->m_targetPath))
    {
        fs::copy_directory(dataStruct->m_targetPath,
                           fs::DEFAULT_SAVE_PATH,
                           dataStruct->m_journalSize,
                           fs::DEFAULT_SAVE_MOUNT,
                           task);
    }
    else if (std::strstr(dataStruct->m_targetPath.cString(), ".zip") != NULL)
    {
        unzFile targetZip = unzOpen64(dataStruct->m_targetPath.cString());
        if (!targetZip)
        {
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 3));
            logger::log("Error opening zip for reading.");
            task->finished();
            return;
        }
        fs::copy_zip_to_directory(targetZip,
                                  fs::DEFAULT_SAVE_PATH,
                                  dataStruct->m_journalSize,
                                  fs::DEFAULT_SAVE_MOUNT,
                                  task);
        unzClose(targetZip);
    }
    else
    {
        fs::copy_file(dataStruct->m_targetPath,
                      fs::DEFAULT_SAVE_PATH,
                      dataStruct->m_journalSize,
                      fs::DEFAULT_SAVE_MOUNT,
                      task);
    }

    // Update this just in case.
    dataStruct->m_spawningState->save_data_written();

    task->finished();
}

static void delete_backup(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct)
{
    if (task)
    {
        task->set_status(strings::get_by_name(strings::names::DELETING_FILES, 0), dataStruct->m_targetPath.cString());
    }

    if (fslib::directoryExists(dataStruct->m_targetPath) &&
        !fslib::deleteDirectoryRecursively(dataStruct->m_targetPath))
    {
        logger::log("Error deleting folder backup: %s", fslib::getErrorString());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 4));
    }
    else if (!fslib::deleteFile(dataStruct->m_targetPath))
    {
        logger::log("Error deleting backup: %s", fslib::getErrorString());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 4));
    }
    dataStruct->m_spawningState->refresh();
    task->finished();
}

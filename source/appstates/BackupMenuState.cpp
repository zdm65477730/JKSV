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
static void createNewBackup(sys::ProgressTask *task,
                            data::User *user,
                            data::TitleInfo *titleInfo,
                            fslib::Path targetPath,
                            BackupMenuState *spawningState);

// Overwrites and existing backup.
static void overwriteBackup(sys::ProgressTask *task, std::shared_ptr<TargetStruct> dataStruct);
// Restores a backup and requires confirmation to do so. Takes a shared_ptr to a TargetStruct.
static void restoreBackup(sys::ProgressTask *task, std::shared_ptr<TargetStruct> dataStruct);
// Deletes a backup and requires confirmation to do so. Takes a shared_ptr to a TargetStruct.
static void deleteBackup(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);

BackupMenuState::BackupMenuState(data::User *user, data::TitleInfo *titleInfo, FsSaveDataType saveType)
    : m_user(user), m_titleInfo(titleInfo), m_saveType(saveType),
      m_directoryPath(config::getWorkingDirectory() / m_titleInfo->getPathSafeTitle())
{
    if (!sm_isInitialized)
    {
        sm_panelWidth = sdl::text::getWidth(22, strings::getByName(strings::names::CONTROL_GUIDES, 2)) + 64;
        // To do: Give classes an alternate so they don't have to be constructed.
        sm_backupMenu = std::make_shared<ui::Menu>(8, 8, sm_panelWidth - 14, 24, 600);
        sm_slidePanel = std::make_unique<ui::SlideOutPanel>(sm_panelWidth, ui::SlideOutPanel::Side::Right);
        sm_menuRenderTarget =
            sdl::TextureManager::createLoadTexture("backupMenuTarget", sm_panelWidth, 600, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
        sm_isInitialized = true;
    }

    // String for the top of the panel.
    std::string panelString = stringutil::getFormattedString("`%s` - %s", m_user->getNickname(), m_titleInfo->getTitle());

    // This needs sm_panelWidth or it'd be in the initializer list.
    sm_slidePanel->pushNewElement(std::make_shared<ui::TextScroll>(panelString, 22, sm_panelWidth, 8, colors::WHITE));


    fslib::Directory saveCheck(fs::DEFAULT_SAVE_PATH);
    m_saveHasData = saveCheck.getCount() > 0;

    BackupMenuState::refresh();
}

BackupMenuState::~BackupMenuState()
{
    sm_slidePanel->clearElements();
}

void BackupMenuState::update(void)
{
    if (input::buttonPressed(HidNpadButton_A) && sm_backupMenu->getSelected() == 0 && m_saveHasData)
    {
        // get name for backup.
        char backupName[0x81] = {0};

        // Set backup to default.
        std::snprintf(backupName, 0x80, "%s - %s", m_user->getPathSafeNickname(), stringutil::getDateString().c_str());

        if (!input::buttonHeld(HidNpadButton_ZR) &&
            !keyboard::getInput(SwkbdType_QWERTY, backupName, strings::getByName(strings::names::KEYBOARD_STRINGS, 0), backupName, 0x80))
        {
            return;
        }
        // To do: This isn't a good way to check for this... Check to make sure zip has zip extension.
        if (config::getByKey(config::keys::EXPORT_TO_ZIP) && std::strstr(backupName, ".zip") == NULL)
        {
            // To do: I should check this.
            std::strcat(backupName, ".zip");
        }
        else if (!config::getByKey(config::keys::EXPORT_TO_ZIP) && !std::strstr(backupName, ".zip") &&
                 !fslib::directoryExists(m_directoryPath / backupName) && !fslib::createDirectory(m_directoryPath / backupName))
        {
            return;
        }
        // Push the task.
        JKSV::pushState(std::make_shared<ProgressState>(createNewBackup, m_user, m_titleInfo, m_directoryPath / backupName, this));
    }
    else if (input::buttonPressed(HidNpadButton_A) && sm_backupMenu->getSelected() == 0 && !m_saveHasData)
    {
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::POP_MESSAGES_BACKUP_MENU, 0));
    }
    else if (input::buttonPressed(HidNpadButton_A) && m_saveHasData && sm_backupMenu->getSelected() > 0)
    {
        int selected = sm_backupMenu->getSelected() - 1;

        std::string queryString =
            stringutil::getFormattedString(strings::getByName(strings::names::BACKUPMENU_CONFIRMATIONS, 0), m_directoryListing[selected]);

        std::shared_ptr<TargetStruct> dataStruct(new TargetStruct);
        dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];

        JKSV::pushState(
            std::make_shared<ConfirmState<sys::ProgressTask, ProgressState, TargetStruct>>(queryString,
                                                                                           config::getByKey(config::keys::HOLD_FOR_OVERWRITE),
                                                                                           overwriteBackup,
                                                                                           dataStruct));
    }
    else if (input::buttonPressed(HidNpadButton_A) && !m_saveHasData && sm_backupMenu->getSelected() > 0)
    {
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::POP_MESSAGES_BACKUP_MENU, 0));
    }
    else if (input::buttonPressed(HidNpadButton_Y) && sm_backupMenu->getSelected() > 0 &&
             (m_saveType != FsSaveDataType_System || config::getByKey(config::keys::ALLOW_WRITING_TO_SYSTEM)))
    {
        // Need to account for new at the top.
        int selected = sm_backupMenu->getSelected() - 1;

        // Gonna need to test this quick.
        fslib::Path targetPath = m_directoryPath / m_directoryListing[selected];

        // This is a quick check to avoid restoring blanks.
        if (fslib::directoryExists(targetPath) && !fs::directoryHasContents(targetPath))
        {
            ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                               strings::getByName(strings::names::POP_MESSAGES_BACKUP_MENU, 1));
            return;
        }
        else if (fslib::fileExists(targetPath) && std::strcmp("zip", targetPath.getExtension()) == 0 && !fs::zipHasContents(targetPath))
        {
            ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                               strings::getByName(strings::names::POP_MESSAGES_BACKUP_MENU, 1));
            return;
        }

        std::shared_ptr<TargetStruct> dataStruct(new TargetStruct);
        dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];
        dataStruct->m_journalSize = m_titleInfo->getJournalSize(m_saveType);
        dataStruct->m_spawningState = this;

        std::string queryString =
            stringutil::getFormattedString(strings::getByName(strings::names::BACKUPMENU_CONFIRMATIONS, 1), m_directoryListing[selected]);

        JKSV::pushState(
            std::make_shared<ConfirmState<sys::ProgressTask, ProgressState, TargetStruct>>(queryString,
                                                                                           config::getByKey(config::keys::HOLD_FOR_RESTORATION),
                                                                                           restoreBackup,
                                                                                           dataStruct));
    }
    else if (input::buttonPressed(HidNpadButton_X) && sm_backupMenu->getSelected() > 0)
    {
        // Selected needs to be offset by one to account for New
        int selected = sm_backupMenu->getSelected() - 1;

        // Create struct to pass.
        std::shared_ptr<TargetStruct> dataStruct(new TargetStruct);
        dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];
        dataStruct->m_spawningState = this;

        // get the string.
        std::string queryString =
            stringutil::getFormattedString(strings::getByName(strings::names::BACKUPMENU_CONFIRMATIONS, 2), m_directoryListing[selected]);

        // Create/push new state.
        JKSV::pushState(std::make_shared<ConfirmState<sys::Task, TaskState, TargetStruct>>(queryString,
                                                                                           config::getByKey(config::keys::HOLD_FOR_DELETION),
                                                                                           deleteBackup,
                                                                                           dataStruct));
    }
    else if (input::buttonPressed(HidNpadButton_B))
    {
        fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);
        sm_slidePanel->close();
    }
    else if (sm_slidePanel->isClosed())
    {
        sm_slidePanel->reset();
        AppState::deactivate();
    }
    // Update panel.
    sm_slidePanel->update(AppState::hasFocus());
    // This state bypasses the Slideout panel's normal behavior because it kind of has to.
    sm_backupMenu->update(AppState::hasFocus());
}

void BackupMenuState::render(void)
{
    // Clear panel target.
    sm_slidePanel->clearTarget();

    // Grab the render target.
    SDL_Texture *slideTarget = sm_slidePanel->get();

    sdl::renderLine(slideTarget, 10, 42, sm_panelWidth - 10, 42, colors::WHITE);
    sdl::renderLine(slideTarget, 10, 648, sm_panelWidth - 10, 648, colors::WHITE);
    sdl::text::render(slideTarget, 32, 673, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, strings::getByName(strings::names::CONTROL_GUIDES, 2));

    // Clear menu target.
    sm_menuRenderTarget->clear(colors::TRANSPARENT);
    // render menu to it.
    sm_backupMenu->render(sm_menuRenderTarget->get(), AppState::hasFocus());
    // render it to panel target.
    sm_menuRenderTarget->render(sm_slidePanel->get(), 0, 43);

    sm_slidePanel->render(NULL, AppState::hasFocus());
}

void BackupMenuState::refresh(void)
{
    m_directoryListing.open(m_directoryPath);
    if (!m_directoryListing)
    {
        return;
    }

    sm_backupMenu->reset();
    sm_backupMenu->addOption(strings::getByName(strings::names::BACKUP_MENU, 0));
    for (int64_t i = 0; i < m_directoryListing.getCount(); i++)
    {
        sm_backupMenu->addOption(m_directoryListing[i]);
    }
}

void BackupMenuState::saveDataWritten(void)
{
    if (!m_saveHasData)
    {
        m_saveHasData = true;
    }
}

// This is the function to create new backups.
static void createNewBackup(sys::ProgressTask *task,
                            data::User *user,
                            data::TitleInfo *titleInfo,
                            fslib::Path targetPath,
                            BackupMenuState *spawningState)
{
    // SaveMeta
    FsSaveDataInfo *saveInfo = user->getSaveInfoByID(titleInfo->getApplicationID());

    // I got tired of typing out the cast.
    fs::SaveMetaData saveMeta = {.m_magic = fs::SAVE_META_MAGIC,
                                 .m_applicationID = titleInfo->getApplicationID(),
                                 .m_saveType = saveInfo->save_data_type,
                                 .m_saveRank = saveInfo->save_data_rank,
                                 .m_saveSpaceID = saveInfo->save_data_space_id,
                                 .m_saveDataSize = titleInfo->getSaveDataSize(saveInfo->save_data_type),
                                 .m_saveDataSizeMax = titleInfo->getSaveDataSizeMax(saveInfo->save_data_type),
                                 .m_journalSize = titleInfo->getJournalSize(saveInfo->save_data_type),
                                 .m_journalSizeMax = titleInfo->getSaveDataSizeMax(saveInfo->save_data_type),
                                 .m_totalSaveSize = fs::getDirectoryTotalSize(fs::DEFAULT_SAVE_PATH)};

    // This extension search is lazy and needs to be revised.
    if (config::getByKey(config::keys::EXPORT_TO_ZIP) || std::strcmp("zip", targetPath.getExtension()) == 0)
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
                                             config::getByKey(config::keys::ZIP_COMPRESSION_LEVEL),
                                             0);
        if (zipError == ZIP_OK)
        {
            zipWriteInFileInZip(newBackup, &saveMeta, sizeof(fs::SaveMetaData));
            zipCloseFileInZip(newBackup);
        }

        fs::copyDirectoryToZip(fs::DEFAULT_SAVE_PATH, newBackup, task);
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

        fs::copyDirectory(fs::DEFAULT_SAVE_PATH, targetPath, 0, {}, task);
    }
    spawningState->refresh();
    task->finished();
}

static void overwriteBackup(sys::ProgressTask *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // DirectoryExists can also be used to check if the target is a directory.
    if (fslib::directoryExists(dataStruct->m_targetPath) && !fslib::deleteDirectoryRecursively(dataStruct->m_targetPath))
    {
        logger::log("Error overwriting backup: %s", fslib::getErrorString());
        task->finished();
        return;
    } // This has an added check for the zip extension so it can't try to overwrite files that aren't supposed to be zip.
    else if (fslib::fileExists(dataStruct->m_targetPath) && std::strcmp("zip", dataStruct->m_targetPath.getExtension()) == 0 &&
             !fslib::deleteFile(dataStruct->m_targetPath))
    {
        logger::log("Error overwriting backup: %s", fslib::getErrorString());
        task->finished();
        return;
    }

    if (std::strcmp("zip", dataStruct->m_targetPath.getExtension()))
    {
        zipFile backupZip = zipOpen64(dataStruct->m_targetPath.cString(), APPEND_STATUS_CREATE);
        fs::copyDirectoryToZip(fs::DEFAULT_SAVE_PATH, backupZip, task);
        zipClose(backupZip, NULL);
    } // I hope this check works for making sure this is a folder
    else if (dataStruct->m_targetPath.getExtension() == nullptr && fslib::createDirectory(dataStruct->m_targetPath))
    {
        fs::copyDirectory(fs::DEFAULT_SAVE_PATH, dataStruct->m_targetPath, 0, {}, task);
    }
    task->finished();
}

static void restoreBackup(sys::ProgressTask *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // Wipe the save root first.
    if (!fslib::deleteDirectoryRecursively(fs::DEFAULT_SAVE_PATH))
    {
        logger::log("Error restoring save: %s", fslib::getErrorString());
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::POP_MESSAGES_BACKUP_MENU, 2));
        task->finished();
        return;
    }

    if (fslib::directoryExists(dataStruct->m_targetPath))
    {
        fs::copyDirectory(dataStruct->m_targetPath, fs::DEFAULT_SAVE_PATH, dataStruct->m_journalSize, fs::DEFAULT_SAVE_MOUNT, task);
    }
    else if (std::strstr(dataStruct->m_targetPath.cString(), ".zip") != NULL)
    {
        unzFile targetZip = unzOpen64(dataStruct->m_targetPath.cString());
        if (!targetZip)
        {
            ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                               strings::getByName(strings::names::POP_MESSAGES_BACKUP_MENU, 3));
            logger::log("Error opening zip for reading.");
            task->finished();
            return;
        }
        fs::copyZipToDirectory(targetZip, fs::DEFAULT_SAVE_PATH, dataStruct->m_journalSize, fs::DEFAULT_SAVE_MOUNT, task);
        unzClose(targetZip);
    }
    else
    {
        fs::copyFile(dataStruct->m_targetPath, fs::DEFAULT_SAVE_PATH, dataStruct->m_journalSize, fs::DEFAULT_SAVE_MOUNT, task);
    }

    // Update this just in case.
    dataStruct->m_spawningState->saveDataWritten();

    task->finished();
}

static void deleteBackup(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct)
{
    if (task)
    {
        task->setStatus(strings::getByName(strings::names::DELETING_FILES, 0), dataStruct->m_targetPath.cString());
    }

    if (fslib::directoryExists(dataStruct->m_targetPath) && !fslib::deleteDirectoryRecursively(dataStruct->m_targetPath))
    {
        logger::log("Error deleting folder backup: %s", fslib::getErrorString());
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::POP_MESSAGES_BACKUP_MENU, 4));
    }
    else if (!fslib::deleteFile(dataStruct->m_targetPath))
    {
        logger::log("Error deleting backup: %s", fslib::getErrorString());
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::POP_MESSAGES_BACKUP_MENU, 4));
    }
    dataStruct->m_spawningState->refresh();
    task->finished();
}

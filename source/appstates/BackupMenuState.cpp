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
#include "stringUtil.hpp"
#include "strings.hpp"
#include "system/system.hpp"
#include "ui/PopMessageManager.hpp"
#include <cstring>

// This struct is used to pass data to Restore, Delete, and upload.
struct TargetStruct
{
        // Path of target.
        fslib::Path m_targetPath;
        // Journal size if commit is needed.
        uint64_t m_journalSize;
        // Spawning state so refresh can be called.
        BackupMenuState *m_spawningState = nullptr;
};

// Declarations here. Definitions after class.
// Create new backup in destinationPath
static void createnewBackup(sys::ProgressTask *task, fslib::Path destinationPath, BackupMenuState *spawningState);
// Restores a backup and requires confirmation to do so. Takes a shared_ptr to a TargetStruct.
static void restoreBackup(sys::ProgressTask *task, std::shared_ptr<TargetStruct> dataStruct);
// Deletes a backup and requires confirmation to do so. Takes a shared_ptr to a TargetStruct.
static void deleteBackup(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);

BackupMenuState::BackupMenuState(data::User *user, data::TitleInfo *titleInfo, FsSaveDataType saveType)
    : m_user(user), m_titleInfo(titleInfo), m_saveType(saveType),
      m_directoryPath(config::getWorkingDirectory() / m_titleInfo->getPathSafeTitle()),
      m_titleWidth(sdl::text::getWidth(22, m_titleInfo->getTitle())), m_titleScrollTimer(3000)
{
    if (!sm_isInitialized)
    {
        m_panelWidth = sdl::text::getWidth(22, strings::getByName(strings::names::CONTROL_GUIDES, 2)) + 64;
        // To do: Give classes an alternate so they don't have to be constructed.
        m_backupMenu = std::make_unique<ui::Menu>(8, 8, m_panelWidth - 24, 24, 600);
        m_slidePanel = std::make_unique<ui::SlideOutPanel>(m_panelWidth, ui::SlideOutPanel::Side::Right);
        m_menuRenderTarget =
            sdl::TextureManager::createLoadTexture("backupMenuTarget", m_panelWidth, 600, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
        sm_isInitialized = true;
    }

    // Check if title needs to be scrolled above the menu or just set the X position.
    if (m_titleWidth >= m_panelWidth)
    {
        m_titleScrolling = true;
        m_titleX = 8;
    }
    else
    {
        m_titleX = (m_panelWidth / 2) - (m_titleWidth / 2);
    }

    // Check if there's currently any data to backup to prevent blanks.
    {
        fslib::Directory saveCheck(fs::DEFAULT_SAVE_PATH);
        logger::log("Save file count: %i", saveCheck.getCount());
        m_saveHasData = saveCheck.getCount() > 0;
    }

    BackupMenuState::refresh();
}

void BackupMenuState::update(void)
{
    if (input::buttonPressed(HidNpadButton_A) && m_backupMenu->getSelected() == 0 && m_saveHasData)
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
        JKSV::pushState(std::make_shared<ProgressState>(createnewBackup, m_directoryPath / backupName, this));
    }
    else if (input::buttonPressed(HidNpadButton_A) && m_backupMenu->getSelected() == 0 && !m_saveHasData)
    {
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS, strings::getByName(strings::names::POP_MESSAGES, 0));
    }
    else if (input::buttonPressed(HidNpadButton_Y) && m_backupMenu->getSelected() > 0 &&
             (m_saveType != FsSaveDataType_System || config::getByKey(config::keys::ALLOW_WRITING_TO_SYSTEM)))
    {
        // Need to account for new at the top.
        int selected = m_backupMenu->getSelected() - 1;

        std::shared_ptr<TargetStruct> dataStruct(new TargetStruct);
        dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];
        dataStruct->m_journalSize = m_titleInfo->getJournalSize(m_saveType);

        std::string queryString =
            stringutil::getFormattedString(strings::getByName(strings::names::BACKUPMENU_CONFIRMATIONS, 0), m_directoryListing[selected]);

        JKSV::pushState(
            std::make_shared<ConfirmState<sys::ProgressTask, ProgressState, TargetStruct>>(queryString,
                                                                                           config::getByKey(config::keys::HOLD_FOR_RESTORATION),
                                                                                           restoreBackup,
                                                                                           dataStruct));
    }
    else if (input::buttonPressed(HidNpadButton_X) && m_backupMenu->getSelected() > 0)
    {
        // Selected needs to be offset by one to account for New
        int selected = m_backupMenu->getSelected() - 1;

        // Create struct to pass.
        std::shared_ptr<TargetStruct> dataStruct(new TargetStruct);
        dataStruct->m_targetPath = m_directoryPath / m_directoryListing[selected];
        dataStruct->m_spawningState = this;

        // get the string.
        std::string queryString =
            stringutil::getFormattedString(strings::getByName(strings::names::BACKUPMENU_CONFIRMATIONS, 1), m_directoryListing[selected]);

        // Create/push new state.
        JKSV::pushState(std::make_shared<ConfirmState<sys::Task, TaskState, TargetStruct>>(queryString,
                                                                                           config::getByKey(config::keys::HOLD_FOR_DELETION),
                                                                                           deleteBackup,
                                                                                           dataStruct));
    }
    else if (input::buttonPressed(HidNpadButton_B))
    {
        fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);
        m_slidePanel->close();
    }
    else if (m_slidePanel->isClosed())
    {
        m_slidePanel->reset();
        AppState::deactivate();
    }

    m_slidePanel->update(AppState::hasFocus());
    // This state bypasses the Slideout panel's normal behavior because it kind of has to.
    m_backupMenu->update(AppState::hasFocus());
}

void BackupMenuState::render(void)
{
    // Clear panel target.
    m_slidePanel->clearTarget();
    // render the current title's name.
    BackupMenuState::renderTitle();
    sdl::renderLine(m_slidePanel->get(), 10, 42, m_panelWidth - 20, 42, colors::WHITE);
    sdl::renderLine(m_slidePanel->get(), 10, 648, m_panelWidth - 20, 648, colors::WHITE);
    sdl::text::render(m_slidePanel->get(),
                      32,
                      673,
                      22,
                      sdl::text::NO_TEXT_WRAP,
                      colors::WHITE,
                      strings::getByName(strings::names::CONTROL_GUIDES, 2));

    // Clear menu target.
    m_menuRenderTarget->clear(colors::TRANSPARENT);
    // render menu to it.
    m_backupMenu->render(m_menuRenderTarget->get(), AppState::hasFocus());
    // render it to panel target.
    m_menuRenderTarget->render(m_slidePanel->get(), 0, 43);
    m_slidePanel->render(NULL, AppState::hasFocus());
}

void BackupMenuState::refresh(void)
{
    m_directoryListing.open(m_directoryPath);
    if (!m_directoryListing.isOpen())
    {
        return;
    }

    m_backupMenu->reset();
    m_backupMenu->addOption(strings::getByName(strings::names::BACKUP_MENU, 0));
    for (int64_t i = 0; i < m_directoryListing.getCount(); i++)
    {
        m_backupMenu->addOption(m_directoryListing[i]);
    }
}

void BackupMenuState::renderTitle(void)
{
    SDL_Texture *SlidePanelTarget = m_slidePanel->get();

    if (m_titleScrolling && m_titleScrollTriggered && m_titleX > -(m_titleWidth + 8))
    {
        m_titleX -= 2;
    }
    else if (m_titleScrolling && m_titleScrollTriggered && m_titleX <= -(m_titleWidth + 8))
    {
        m_titleX = 8;
        m_titleScrollTriggered = false;
        m_titleScrollTimer.restart();
    }
    else if (m_titleScrolling && m_titleScrollTimer.isTriggered())
    {
        m_titleScrollTriggered = true;
    }

    if (m_titleScrolling && m_titleScrollTriggered)
    {
        // This is just a trick, or maybe the only way to accomplish this. Either way, it works.
        // render title first time.
        sdl::text::render(SlidePanelTarget, m_titleX, 8, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_titleInfo->getTitle());
        // render it again following the first.
        sdl::text::render(SlidePanelTarget,
                          m_titleX + m_titleWidth + 16,
                          8,
                          22,
                          sdl::text::NO_TEXT_WRAP,
                          colors::WHITE,
                          m_titleInfo->getTitle());
    }
    else
    {
        sdl::text::render(SlidePanelTarget, m_titleX, 8, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_titleInfo->getTitle());
    }
}

// This is the function to create new backups.
static void createnewBackup(sys::ProgressTask *task, fslib::Path destinationPath, BackupMenuState *spawningState)
{
    // This extension search is lazy and needs to be revised.
    if (config::getByKey(config::keys::EXPORT_TO_ZIP) || std::strstr(destinationPath.cString(), ".zip") != NULL)
    {
        zipFile newBackup = zipOpen64(destinationPath.cString(), APPEND_STATUS_CREATE);
        fs::copyDirectoryToZip(fs::DEFAULT_SAVE_PATH, newBackup, task);
        zipClose(newBackup, NULL);
    }
    else
    {
        fs::copyDirectory(fs::DEFAULT_SAVE_PATH, destinationPath, 0, {}, task);
    }
    spawningState->refresh();
    task->finished();
}

static void restoreBackup(sys::ProgressTask *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // Wipe the save root first.
    if (!fslib::deleteDirectoryRecursively(fs::DEFAULT_SAVE_PATH))
    {
        logger::log("Error restoring save: %s", fslib::getErrorString());
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
    }
    else if (!fslib::deleteFile(dataStruct->m_targetPath))
    {
        logger::log("Error deleting backup: %s", fslib::getErrorString());
    }
    dataStruct->m_spawningState->refresh();
    task->finished();
}

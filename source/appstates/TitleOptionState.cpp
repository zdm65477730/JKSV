#include "appstates/TitleOptionState.hpp"
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
    const char ERROR_RESETTING_SAVE = "Error resetting save data: %s";
} // namespace

// Struct to send data to functions that require confirmation.
typedef struct
{
        data::User *m_targetUser;
        data::TitleInfo *m_targetTitle;
} TargetStruct;

// Declarations. Definitions after class.
// I don't like this, but it needs to be like this to be usable with confirmation.
static void blacklistTitle(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);
static void deleteAllBackupsForTitle(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);
static void resetSaveData(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);
static void deleteSaveDataFromSystem(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct);
static void changeOutputPath(data::TitleInfo *targetTitle);

TitleOptionState::TitleOptionState(data::User *user, data::TitleInfo *titleInfo) : m_targetUser(user), m_titleInfo(titleInfo)
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
        while ((currentString = strings::getByName(strings::names::TITLE_OPTIONS, stringIndex++)) != nullptr)
        {
            sm_titleOptionMenu->addOption(currentString);
        }
        // Only do this once.
        sm_initialized = true;
    }
}

void TitleOptionState::update(void)
{
    // Update panel and menu.
    sm_slidePanel->update(AppState::hasFocus());
    sm_titleOptionMenu->update(AppState::hasFocus());

    if (input::buttonPressed(HidNpadButton_A))
    {
        switch (sm_titleOptionMenu->getSelected())
        {
            case BLACKLIST:
            {
            }
            break;

            case CHANGE_OUTPUT:
            {
                changeOutputPath(m_titleInfo);
            }
            break;

            case DELETE_ALL_BACKUPS:
            {
            }
        }
    }
    else if (input::buttonPressed(HidNpadButton_B))
    {
        sm_slidePanel->close();
    }
    else if (sm_slidePanel->isClosed())
    {
        // Reset static members.
        sm_slidePanel->reset();
        sm_titleOptionMenu->setSelected(0);
        AppState::deactivate();
    }
}

void TitleOptionState::render(void)
{
    sm_slidePanel->clearTarget();
    sm_titleOptionMenu->render(sm_slidePanel->get(), AppState::hasFocus());
    sm_slidePanel->render(NULL, AppState::hasFocus());
}

static void blacklistTitle(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // We're not gonna bother with a status for this. It'll flicker, but be barely noticeable.
    config::addRemoveBlacklist(dataStruct->m_targetTitle->getApplicationID());
    task->finished();
}

static void deleteAllBackupsForTitle(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // Get the path.
    fslib::Path titlePath = config::getWorkingDirectory() / dataStruct->m_targetTitle->getPathSafeTitle();

    // Set the status.
    task->setStatus(strings::getByName(strings::names::TITLE_OPTION_STATUS, 0), dataStruct->m_targetTitle->getTitle());

    // Just call this and nuke the folder.
    if (!fslib::deleteDirectoryRecursively(titlePath))
    {
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::TITLE_OPTION_POPS, 1));
    }
    else
    {
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::TITLE_OPTION_POPS, 0),
                                           dataStruct->m_targetTitle->getTitle());
    }
    task->finished();
}

static void resetSaveData(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // Attempt to mount save.
    if (!fslib::openSaveFileSystemWithSaveDataInfo(fs::DEFAULT_SAVE_MOUNT,
                                                   *dataStruct->m_targetUser->getSaveInfoByID(dataStruct->m_targetTitle->getApplicationID())))
    {
        logger::log(ERROR_RESETTING_SAVE, fslib::getErrorString());
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::TITLE_OPTION_POPS, 2));
        task->finished();
        return;
    }

    // Wipe the root.
    if (!fslib::deleteDirectoryRecursively(fs::DEFAULT_SAVE_PATH))
    {
        fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);
        logger::log(ERROR_RESETTING_SAVE, fslib::getErrorString());
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::TITLE_OPTION_POPS, 2));
        task->finished();
        return;
    }

    // Attempt commit.
    if (!fslib::commitDataToFileSystem(fs::DEFAULT_SAVE_MOUNT))
    {
        fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);
        logger(ERROR_RESETTING_SAVE, fslib::getErrorString());
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::TITLE_OPTION_POPS, 2));
        task->finished();
        return;
    }

    // Should be good to go.
    fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);
    ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS, strings::getByName(strings::names::TITLE_OPTION_POPS, 3));
    task->finished();
}

static void deleteSaveDataFromSystem(sys::Task *task, std::shared_ptr<TargetStruct> dataStruct)
{
    // Set status. We're going to borrow this string from the other state's strings.
    task->setStatus(strings::getByName(strings::names::USER_OPTION_STATUS, 1), dataStruct->m_targetTitle->getTitle());
}

static void changeOutputPath(data::TitleInfo *targetTitle)
{
    // This is where we're writing the path.
    char pathBuffer[0x200] = {0};

    // Header string.
    std::string headerString = stringutil::getFormattedString(strings::getByName(strings::names::KEYBOARD_STRINGS, 7), targetTitle->getTitle());

    // Try to get input.
    if (!keyboard::getInput(SwkbdType_QWERTY, targetTitle->getPathSafeTitle(), headerString, pathBuffer, 0x200))
    {
        return;
    }

    // Try to make sure it will work.
    if (!stringutil::sanitizeStringForPath(pathBuffer, pathBuffer, 0x200))
    {
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::POP_MESSAGES_TITLE_OPTIONS, 0));
        return;
    }

    // Rename folder to match so there are no issues.
    fslib::Path oldPath = config::getWorkingDirectory() / targetTitle->getPathSafeTitle();
    fslib::Path newPath = config::getWorkingDirectory() / pathBuffer;
    if (fslib::directoryExists(oldPath) && !fslib::renameDirectory(oldPath, newPath))
    {
        // Bail if this fails, because something is really wrong.
        logger::log("Error setting new output path: %s", fslib::getErrorString());
        return;
    }

    // Add it to config and set target title to use it.
    targetTitle->setPathSafeTitle(pathBuffer, std::strlen(pathBuffer));
    config::addCustomPath(targetTitle->getApplicationID(), pathBuffer);

    // Pop so we know stuff happened.
    ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                       strings::getByName(strings::names::POP_MESSAGES_TITLE_OPTIONS, 1),
                                       pathBuffer);
}

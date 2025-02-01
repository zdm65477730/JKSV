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
static void backupAllForUser(sys::ProgressTask *task, std::shared_ptr<UserStruct> dataStruct);
// // Creates all save data for the current user.
static void createAllSaveDataForUser(sys::Task *task, std::shared_ptr<UserStruct> dataStruct);
// // Deletes all save data from the system for the target user.
static void deleteAllSaveDataForUser(sys::Task *task, std::shared_ptr<UserStruct> dataStruct);

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
    while ((currentString = strings::getByName(strings::names::USER_OPTIONS, currentStringIndex++)) != nullptr)
    {
        m_userOptionMenu.addOption(stringutil::getFormattedString(currentString, m_user->getNickname()));
    }
}

void UserOptionState::update(void)
{
    m_menuPanel->update(AppState::hasFocus());

    if (input::buttonPressed(HidNpadButton_A) && m_user->getAccountSaveType() != FsSaveDataType_System)
    {
        switch (m_userOptionMenu.getSelected())
        {
            case BACKUP_ALL:
            {
                // This is broken down to make it easier to read.
                std::string queryString =
                    stringutil::getFormattedString(strings::getByName(strings::names::USER_OPTION_CONFIRMATIONS, 0), m_user->getNickname());

                // Data to send if confirmed.
                std::shared_ptr<UserStruct> dataStruct(new UserStruct);
                dataStruct->m_targetUser = m_user;

                // State to push
                std::shared_ptr<ConfirmState<sys::ProgressTask, ProgressState, UserStruct>> confirmBackupAll =
                    std::make_shared<ConfirmState<sys::ProgressTask, ProgressState, UserStruct>>(queryString,
                                                                                                 false,
                                                                                                 backupAllForUser,
                                                                                                 dataStruct);

                JKSV::pushState(confirmBackupAll);
            }
            break;

            case CREATE_SAVE:
            {
                // This just pushes the state with the menu to select.
                JKSV::pushState(std::make_shared<SaveCreateState>(m_user, m_titleSelect));
            }
            break;

            case CREATE_ALL_SAVE:
            {
                std::string queryString =
                    stringutil::getFormattedString(strings::getByName(strings::names::USER_OPTION_CONFIRMATIONS, 1), m_user->getNickname());

                std::shared_ptr<UserStruct> dataStruct(new UserStruct);
                dataStruct->m_targetUser = m_user;

                std::shared_ptr<ConfirmState<sys::Task, TaskState, UserStruct>> confirmCreateAll =
                    std::make_shared<ConfirmState<sys::Task, TaskState, UserStruct>>(queryString, true, createAllSaveDataForUser, dataStruct);

                // Done?
                JKSV::pushState(confirmCreateAll);
            }
            break;

            case DELETE_ALL_SAVE:
            {
                std::string queryString =
                    stringutil::getFormattedString(strings::getByName(strings::names::USER_OPTION_CONFIRMATIONS, 2), m_user->getNickname());

                std::shared_ptr<UserStruct> dataStruct(new UserStruct);
                dataStruct->m_targetUser = m_user;

                std::shared_ptr<ConfirmState<sys::Task, TaskState, UserStruct>> confirmDeleteAll =
                    std::make_shared<ConfirmState<sys::Task, TaskState, UserStruct>>(queryString, true, deleteAllSaveDataForUser, dataStruct);

                JKSV::pushState(confirmDeleteAll);
            }
            break;
        }
    }
    else if (input::buttonPressed(HidNpadButton_B))
    {
        m_menuPanel->close();
    }
    else if (m_menuPanel->isClosed())
    {
        AppState::deactivate();
        m_menuPanel->reset();
    }

    m_userOptionMenu.update(AppState::hasFocus());
}

void UserOptionState::render(void)
{
    // Render target user's title selection screen.
    m_titleSelect->render();

    // Render panel.
    m_menuPanel->clearTarget();
    m_userOptionMenu.render(m_menuPanel->get(), AppState::hasFocus());
    m_menuPanel->render(NULL, AppState::hasFocus());
}

static void backupAllForUser(sys::ProgressTask *task, std::shared_ptr<UserStruct> dataStruct)
{
    data::User *targetUser = dataStruct->m_targetUser;

    for (size_t i = 0; i < targetUser->getTotalDataEntries(); i++)
    {
        // This should be safe like this....
        FsSaveDataInfo *currentSaveInfo = targetUser->getSaveInfoAt(i);
        data::TitleInfo *currentTitle = data::getTitleInfoByID(currentSaveInfo->application_id);

        if (!currentSaveInfo || !currentTitle)
        {
            continue;
        }

        // Try to create target game folder.
        fslib::Path gameFolder = config::getWorkingDirectory() / currentTitle->getPathSafeTitle();
        if (!fslib::directoryExists(gameFolder) && !fslib::createDirectory(gameFolder))
        {
            continue;
        }

        // Try to mount save data.
        bool saveMounted = fslib::openSaveFileSystemWithSaveDataInfo(fs::DEFAULT_SAVE_MOUNT, *targetUser->getSaveInfoAt(i));

        // Check to make sure the save actually has data to avoid blanks.
        {
            fslib::Directory saveCheck(fs::DEFAULT_SAVE_PATH);
            if (saveMounted && saveCheck.getCount() <= 0)
            {
                // Gonna borrow these messages. No point in repeating them.
                ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                   strings::getByName(strings::names::POP_MESSAGES_BACKUP_MENU, 0));
                fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);
                continue;
            }
        }

        if (currentTitle && saveMounted && config::getByKey(config::keys::EXPORT_TO_ZIP))
        {
            fslib::Path targetPath = config::getWorkingDirectory() / currentTitle->getPathSafeTitle() / targetUser->getPathSafeNickname() +
                                     " - " + stringutil::getDateString() + ".zip";

            zipFile targetZip = zipOpen64(targetPath.cString(), APPEND_STATUS_CREATE);
            if (!targetZip)
            {
                logger::log("Error creating zip: %s", fslib::getErrorString());
                continue;
            }
            fs::copyDirectoryToZip(fs::DEFAULT_SAVE_PATH, targetZip, task);
            zipClose(targetZip, NULL);
        }
        else if (currentTitle && saveMounted)
        {
            fslib::Path targetPath = config::getWorkingDirectory() / currentTitle->getPathSafeTitle() / targetUser->getPathSafeNickname() +
                                     " - " + stringutil::getDateString();

            if (!fslib::createDirectory(targetPath))
            {
                logger::log("Error creating backup directory: %s", fslib::getErrorString());
                continue;
            }
            fs::copyDirectory(fs::DEFAULT_SAVE_PATH, targetPath, 0, {}, task);
        }

        if (saveMounted)
        {
            fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);
        }
    }
    task->finished();
}

static void createAllSaveDataForUser(sys::Task *task, std::shared_ptr<UserStruct> dataStruct)
{
    data::User *targetUser = dataStruct->m_targetUser;

    // Get title info map.
    auto &titleInfoMap = data::getTitleInfoMap();

    // Iterate through it.
    for (auto &[applicationID, titleInfo] : titleInfoMap)
    {
        if (!titleInfo.hasSaveDataType(targetUser->getAccountSaveType()))
        {
            continue;
        }

        // Set status.
        task->setStatus(strings::getByName(strings::names::USER_OPTION_STATUS, 0), titleInfo.getTitle());

        if (!fs::createSaveDataFor(targetUser, &titleInfo))
        {
            // Function should log error too.
            ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                               strings::getByName(strings::names::POP_MESSAGES_SAVE_CREATE, 2));
        }
    }
    task->finished();
}

static void deleteAllSaveDataForUser(sys::Task *task, std::shared_ptr<UserStruct> dataStruct)
{
    data::User *targetUser = dataStruct->m_targetUser;

    for (size_t i = 0; i < targetUser->getTotalDataEntries(); i++)
    {
        // Grab title for title.
        const char *targetTitle = data::getTitleInfoByID(targetUser->getApplicationIDAt(i))->getTitle();

        // Update thread task.
        task->setStatus(strings::getByName(strings::names::USER_OPTION_STATUS, 1), targetTitle);

        if (!fs::deleteSaveData(*targetUser->getSaveInfoAt(i)))
        {
            ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                               strings::getByName(strings::names::POP_MESSAGES_SAVE_CREATE, 2));
        }
    }
    task->finished();
}

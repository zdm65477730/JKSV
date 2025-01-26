#include "appstates/UserOptionState.hpp"
#include "JKSV.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/SaveCreateState.hpp"
#include "appstates/TaskState.hpp"
#include "config.hpp"
#include "data/data.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "stringUtil.hpp"
#include "strings.hpp"
#include "system/system.hpp"
#include "ui/PopMessageManager.hpp"

// Declarations here. Defintions after class.
// Backs up all save data for the target user.
static void backupAllForUser(sys::ProgressTask *task, data::User *targetUser);
// // Creates all save data for the current user.
// static void createAllSaveDataForUser(sys::Task *task, data::User *targetUser);
// // Deletes all save data from the system for the target user.
// static void deleteAllSaveDataForUser(sys::Task *task, data::User *targetUser);

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
            case 0:
            {
                JKSV::pushState(std::make_shared<ProgressState>(backupAllForUser, m_user));
            }
            break;

            case 1:
            {
                JKSV::pushState(std::make_shared<SaveCreateState>(m_user, m_titleSelect));
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

static void backupAllForUser(sys::ProgressTask *task, data::User *targetUser)
{
    for (size_t i = 0; i < targetUser->getTotalDataEntries(); i++)
    {
        // This should be safe like this....
        FsSaveDataInfo *currentSaveInfo = targetUser->getSaveInfoAt(i);
        data::TitleInfo *currentTitle = data::getTitleInfoByID(currentSaveInfo->application_id);

        if (!currentSaveInfo || !currentTitle)
        {
            logger::log("One of these is nullptr?");
            continue;
        }

        // Try to create target game folder.
        fslib::Path gameFolder = config::getWorkingDirectory() / currentTitle->getPathSafeTitle();
        if (!fslib::directoryExists(gameFolder) && !fslib::createDirectory(gameFolder))
        {
            logger::log("Error creating target game folder: %s", fslib::getErrorString());
            continue;
        }

        // Try to mount save data.
        bool saveMounted = fslib::openSaveFileSystemWithSaveDataInfo(fs::DEFAULT_SAVE_MOUNT, *targetUser->getSaveInfoAt(i));

        // Check to make sure the save actually has data to avoid blanks.
        {
            fslib::Directory saveCheck(fs::DEFAULT_SAVE_PATH);
            if (saveMounted && saveCheck.getCount() <= 0)
            {
                ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                   strings::getByName(strings::names::POP_MESSAGES, 0));
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

// static void createAllSaveDataForUser(sys::Task *task, data::User *targetUser)
// {
//     // Get title info map.
//     auto &titleInfoMap = data::getTitleInfoMap();

//     // Iterate through it.
//     for (auto &[applicationID, titleInfo] : titleInfoMap)
//     {
//         // Only continue if the info has save data for the type the account is.
//         if (titleInfo.hasSaveDataType(targetUser->getAccountSaveType()))
//         {
//         }
//     }
// }

// static void deleteAllSaveDataForUser(sys::Task *task, data::User *targetUser)
// {
// }

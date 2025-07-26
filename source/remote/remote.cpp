#include "remote/remote.hpp"

#include "StateManager.hpp"
#include "appstates/TaskState.hpp"
#include "logger.hpp"
#include "remote/GoogleDrive.hpp"
#include "remote/WebDav.hpp"
#include "strings.hpp"
#include "ui/PopMessageManager.hpp"

#include <chrono>
#include <ctime>
#include <memory>
#include <thread>

namespace
{
    /// @brief This is just the string for finding and creating the JKSV dir.
    const char *STRING_JKSV_DIR = "JKSV";

    /// @brief This is the single (for now) instance of a storage class.
    std::unique_ptr<remote::Storage> s_storage{};
} // namespace

// Declarations here. Definitions at bottom.
/// @brief This is the thread function that handles logging into Google.
static void drive_sign_in(sys::Task *task, remote::GoogleDrive *drive);

/// @brief This creates (if needed) the JKSV folder for Google Drive and sets it as the root.
/// @param drive Pointer to the drive instance..
static void drive_set_jksv_root(remote::GoogleDrive *drive);

void remote::initialize_google_drive()
{
    // Create drive instance.
    s_storage = std::make_unique<remote::GoogleDrive>();

    // Need to cast this for it to work right.
    remote::GoogleDrive *drive = static_cast<remote::GoogleDrive *>(s_storage.get());

    if (drive->sign_in_required())
    {
        auto signIn = std::make_shared<TaskState>(drive_sign_in, drive);
        StateManager::push_state(signIn);
        // We can return here because the task should handle the rest of the setup for us.
        return;
    }

    // To do: Handle this better. Maybe retry somehow?
    if (!drive->is_initialized()) { return; }
    // Can't forget this.
    drive_set_jksv_root(drive);
    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                        strings::get_by_name(strings::names::GOOGLE_DRIVE, 1));
}

void remote::initialize_webdav()
{
    s_storage = std::make_unique<remote::WebDav>();

    if (s_storage->is_initialized())
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                            strings::get_by_name(strings::names::WEBDAV, 0));
    }
    else
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                            strings::get_by_name(strings::names::WEBDAV, 1));
    }
}

remote::Storage *remote::get_remote_storage()
{
    if (!s_storage || !s_storage->is_initialized()) { return nullptr; }
    return s_storage.get();
}

static void drive_sign_in(sys::Task *task, remote::GoogleDrive *drive)
{
    static const char *STRING_ERROR_SIGNING_IN = "Error signing into Google Drive: %s";

    std::string message{}, deviceCode{};
    std::time_t expiration = 0;
    int pollingInterval    = 0;

    if (!drive->get_sign_in_data(message, deviceCode, expiration, pollingInterval))
    {
        logger::log(STRING_ERROR_SIGNING_IN, "Getting sign in data failed!");
        task->finished();
        return;
    }

    task->set_status(message.c_str());

    while (std::time(NULL) < expiration && !drive->poll_sign_in(deviceCode))
    {
        std::this_thread::sleep_for(std::chrono::seconds(pollingInterval));
    }

    if (drive->is_initialized())
    {
        // Run this quick so the root is set correctly.
        drive_set_jksv_root(drive);
        // Show everyone I did it!
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                            strings::get_by_name(strings::names::GOOGLE_DRIVE, 1));
    }
    else
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS,
                                            strings::get_by_name(strings::names::GOOGLE_DRIVE, 2));
    }

    task->finished();
}

static void drive_set_jksv_root(remote::GoogleDrive *drive)
{
    static const char *STRING_ERROR_SETTING_DIR = "Error creating/setting JKSV directory on Drive: %s";

    if (!drive->directory_exists(STRING_JKSV_DIR) && !drive->create_directory(STRING_JKSV_DIR))
    {
        logger::log(STRING_ERROR_SETTING_DIR, "Error finding and/or creating directory!");
        return;
    }

    remote::Item *jksvDir = drive->get_directory_by_name(STRING_JKSV_DIR);
    if (!jksvDir)
    {
        logger::log(STRING_ERROR_SETTING_DIR, "Error locating directory in list! This shouldn't be able to happen!");
        return;
    }
    drive->set_root_directory(jksvDir);
    drive->change_directory(jksvDir);
}

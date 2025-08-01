#include "remote/remote.hpp"

#include "StateManager.hpp"
#include "appstates/TaskState.hpp"
#include "error.hpp"
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

bool remote::has_internet_connection()
{
    NifmInternetConnectionType type{};
    uint32_t strength{};
    NifmInternetConnectionStatus status{};
    const bool getError = error::libnx(nifmGetInternetConnectionStatus(&type, &strength, &status));
    if (getError || status != NifmInternetConnectionStatus_Connected) { return false; }
    return true;
}

void remote::initialize_google_drive()
{
    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;

    if (!remote::has_internet_connection())
    {
        const char *popNoInternet = strings::get_by_name(strings::names::REMOTE_POPS, 0);
        ui::PopMessageManager::push_message(popTicks, popNoInternet);
        return;
    }

    s_storage                  = std::make_unique<remote::GoogleDrive>();
    remote::GoogleDrive *drive = static_cast<remote::GoogleDrive *>(s_storage.get());
    if (drive->sign_in_required())
    {
        TaskState::create_and_push(drive_sign_in, drive);
        return;
    }

    // To do: Handle this better. Maybe retry somehow?
    if (!drive->is_initialized()) { return; }

    drive_set_jksv_root(drive);
    const char *popDriveSuccess = strings::get_by_name(strings::names::GOOGLE_DRIVE, 1);
    ui::PopMessageManager::push_message(popTicks, popDriveSuccess);
}

void remote::initialize_webdav()
{
    if (!remote::has_internet_connection()) { return; }

    s_storage          = std::make_unique<remote::WebDav>();
    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;
    if (s_storage->is_initialized())
    {
        const char *popDavSuccess = strings::get_by_name(strings::names::WEBDAV, 0);
        ui::PopMessageManager::push_message(popTicks, popDavSuccess);
    }
    else
    {
        const char *popDavFailed = strings::get_by_name(strings::names::WEBDAV, 1);
        ui::PopMessageManager::push_message(popTicks, popDavFailed);
    }
}

remote::Storage *remote::get_remote_storage()
{
    if (!s_storage || !s_storage->is_initialized()) { return nullptr; }
    return s_storage.get();
}

static void drive_sign_in(sys::Task *task, remote::GoogleDrive *drive)
{
    static constexpr const char *STRING_ERROR_SIGNING_IN = "Error signing into Google Drive: %s";

    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;
    std::string message{}, deviceCode{};
    std::time_t expiration{};
    int pollingInterval{};
    if (!drive->get_sign_in_data(message, deviceCode, expiration, pollingInterval))
    {
        logger::log(STRING_ERROR_SIGNING_IN, "Getting sign in data failed!");
        TASK_FINISH_RETURN(task);
    }

    task->set_status(message.c_str());

    while (std::time(NULL) < expiration && !drive->poll_sign_in(deviceCode))
    {
        std::this_thread::sleep_for(std::chrono::seconds(pollingInterval));
    }

    if (drive->is_initialized())
    {
        drive_set_jksv_root(drive);
        const char *popDriveSuccess = strings::get_by_name(strings::names::GOOGLE_DRIVE, 1);
        ui::PopMessageManager::push_message(popTicks, popDriveSuccess);
    }
    else
    {
        const char *popDriveFailed = strings::get_by_name(strings::names::GOOGLE_DRIVE, 2);
        ui::PopMessageManager::push_message(popTicks, popDriveFailed);
    }

    task->finished();
}

static void drive_set_jksv_root(remote::GoogleDrive *drive)
{
    const bool jksvExists  = drive->directory_exists(STRING_JKSV_DIR);
    const bool jksvCreated = !jksvExists && drive->create_directory(STRING_JKSV_DIR);
    if (!jksvExists && !jksvCreated) { return; }

    const remote::Item *jksvDir = drive->get_directory_by_name(STRING_JKSV_DIR);
    if (!jksvDir) { return; }

    drive->set_root_directory(jksvDir);
    drive->change_directory(jksvDir);
}

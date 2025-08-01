#include "tasks/useroptions.hpp"

#include "config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "remote/remote.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "tasks/backup.hpp"
#include "ui/ui.hpp"

void tasks::useroptions::backup_all_for_user_local(sys::ProgressTask *task, UserOptionState::TaskData taskData, bool killTask)
{
    if (error::is_null(task)) { return; }

    data::User *user = taskData->user;
    if (error::is_null(user)) { TASK_FINISH_RETURN(task); }

    const fslib::Path workDir = config::get_working_directory();
    const bool exportToZip    = config::get_by_key(config::keys::EXPORT_TO_ZIP);
    const int titleCount      = user->get_total_data_entries();
    for (int i = 0; i < titleCount; i++)
    {
        const FsSaveDataInfo *saveInfo = user->get_save_info_at(i);
        if (error::is_null(saveInfo)) { continue; }

        {
            fs::ScopedSaveMount scopedMount{fs::DEFAULT_SAVE_MOUNT, saveInfo};
            if (!scopedMount.is_open() || !fs::directory_has_contents(fs::DEFAULT_SAVE_ROOT)) { continue; }
        }

        data::TitleInfo *titleInfo = data::get_title_info_by_id(saveInfo->application_id);
        if (error::is_null(titleInfo)) { continue; }

        const fslib::Path targetDir{workDir / titleInfo->get_path_safe_title()};
        const bool targetExists = fslib::directory_exists(targetDir);
        const bool createError  = !targetExists && error::fslib(fslib::create_directories_recursively(targetDir));
        if (!targetExists && createError) { continue; }

        fslib::Path target{targetDir / user->get_path_safe_nickname() + " - " + stringutil::get_date_string()};
        if (exportToZip) { target += ".zip"; }

        tasks::backup::create_new_backup_local(task, user, titleInfo, target, nullptr, false);
    }

    if (killTask) { task->finished() };
}

void tasks::useroptions::backup_all_for_user_remote(sys::ProgressTask *task, UserOptionState::TaskData taskData, bool killTask)
{
    if (error::is_null(task)) { return; }

    data::User *user        = taskData->user;
    remote::Storage *remote = remote::get_remote_storage();
    if (error::is_null(user) || error::is_null(remote)) { TASK_FINISH_RETURN(task); }

    const int titleCount = user->get_total_data_entries();
    for (int i = 0; i < titleCount; i++)
    {
        const FsSaveDataInfo *saveInfo = user->get_save_info_at(i);
        if (error::is_null(saveInfo)) { continue; }

        {
            fs::ScopedSaveMount scopedMount{fs::DEFAULT_SAVE_MOUNT, saveInfo};
            if (!scopedMount.is_open() || !fs::directory_has_contents(fs::DEFAULT_SAVE_ROOT)) { continue; }
        }

        data::TitleInfo *titleInfo = data::get_title_info_by_id(saveInfo->application_id);
        if (error::is_null(saveInfo)) { continue; }

        const std::string_view remoteTitle =
            remote->supports_utf8() ? titleInfo->get_title() : titleInfo->get_path_safe_title();
        const bool dirExists = remote->directory_exists(remoteTitle);
        const bool created   = !dirExists && remote->create_directory(remoteTitle);
        if (!dirExists && !created) { continue; }

        remote::Item *target = remote->get_directory_by_name(remoteTitle);
        remote->change_directory(target);

        const std::string dateString = stringutil::get_date_string();
        const std::string remoteName =
            stringutil::get_formatted_string("%s - %s.zip", user->get_nickname(), dateString.c_str());
        tasks::backup::create_new_backup_remote(task, user, titleInfo, remoteName, nullptr, false);

        remote->return_to_root();
    }

    if (killTask) { task->finished() };
}

void tasks::useroptions::create_all_save_data_for_user(sys::Task *task, UserOptionState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    data::User *user               = taskData->user;
    UserOptionState *spawningState = taskData->spawningState;
    if (error::is_null(user) || error::is_null(spawningState)) { TASK_FINISH_RETURN(task); }

    auto &titleInfoMap       = data::get_title_info_map();
    const int popTicks       = ui::PopMessageManager::DEFAULT_TICKS;
    const char *statusFormat = strings::get_by_name(strings::names::USEROPTION_STATUS, 0);
    const char *popFailure   = strings::get_by_name(strings::names::SAVECREATE_POPS, 1);
    const uint8_t saveType   = user->get_account_save_type();

    for (auto &[applicationID, titleInfo] : titleInfoMap)
    {
        const char *title      = titleInfo.get_title();
        const bool hasSaveType = titleInfo.has_save_data_type(saveType);
        if (!hasSaveType) { continue; }

        {
            const std::string status = stringutil::get_formatted_string(statusFormat, title);
            task->set_status(status);
        }

        const bool saveCreated = fs::create_save_data_for(user, &titleInfo);
        if (!saveCreated)
        {
            const std::string popMessage = stringutil::get_formatted_string(popFailure, title);
            ui::PopMessageManager::push_message(popTicks, popMessage);
        }
    }

    spawningState->refresh_required();
    task->finished();
}

void tasks::useroptions::delete_all_save_data_for_user(sys::Task *task, UserOptionState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    data::User *user               = taskData->user;
    UserOptionState *spawningState = taskData->spawningState;
    if (error::is_null(user) || error::is_null(spawningState)) { TASK_FINISH_RETURN(task); }
    if (user->get_account_save_type() == FsSaveDataType_System) { TASK_FINISH_RETURN(task); }

    const int popTicks       = ui::PopMessageManager::DEFAULT_TICKS;
    const char *statusFormat = strings::get_by_name(strings::names::USEROPTION_STATUS, 1);
    const char *popFailed    = strings::get_by_name(strings::names::SAVECREATE_POPS, 2);

    std::vector<uint64_t> applicationIDs{};
    const int titleCount = user->get_total_data_entries();
    for (int i = 0; i < titleCount; i++)
    {
        const FsSaveDataInfo *saveInfo = user->get_save_info_at(i);
        if (error::is_null(saveInfo) || saveInfo->save_data_type == FsSaveDataType_System) { continue; }

        const uint64_t applicationID = saveInfo->application_id;
        {
            data::TitleInfo *titleInfo = data::get_title_info_by_id(applicationID);
            const char *title          = titleInfo->get_title();
            const std::string status   = stringutil::get_formatted_string(statusFormat, title);
            task->set_status(status);
        }

        const bool saveDeleted = fs::delete_save_data(saveInfo);
        if (!saveDeleted)
        {
            ui::PopMessageManager::push_message(popTicks, popFailed);
            continue;
        }
        applicationIDs.push_back(applicationID);
    }

    for (const uint64_t &applicationID : applicationIDs) { user->erase_save_info_by_id(applicationID); }
    spawningState->refresh_required();
    task->finished();
}

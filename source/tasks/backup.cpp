#include "tasks/backup.hpp"

#include "config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "logger.hpp"
#include "remote/remote.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"

#include <cstring>

namespace
{
    constexpr const char *STRING_ZIP_EXT   = ".zip";
    constexpr const char *STRING_JKSV_TEMP = "sdmc:/jksvTemp.zip"; // This is name this so if something fails, people know.
    constexpr size_t SIZE_SAVE_META        = sizeof(fs::SaveMetaData);
}

// Definitions at bottom.
static bool read_and_process_meta(const fslib::Path &targetDir, BackupMenuState::TaskData taskData, sys::ProgressTask *task);
static bool read_and_process_meta(fs::MiniUnzip &unzip, BackupMenuState::TaskData taskData, sys::ProgressTask *task);
static fs::ScopedSaveMount create_scoped_mount(const FsSaveDataInfo *saveInfo);

void tasks::backup::create_new_backup_local(sys::ProgressTask *task,
                                            data::User *user,
                                            data::TitleInfo *titleInfo,
                                            fslib::Path target,
                                            BackupMenuState *spawningState,
                                            bool killTask)
{
    if (error::is_null(task)) { return; }

    const bool hasZipExt           = std::strstr(target.full_path(), STRING_ZIP_EXT);
    const uint64_t applicationID   = titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(applicationID);

    if (error::is_null(saveInfo))
    {
        task->finished();
        return;
    }

    fs::SaveMetaData saveMeta{};
    const bool hasValidMeta = fs::fill_save_meta_data(saveInfo, saveMeta);
    if (hasZipExt) // At this point, this should have the zip extension appended if needed.
    {
        fs::MiniZip zip{target};
        if (!zip.is_open())
        {
            task->finished();
            return;
        }

        const bool openMeta = hasValidMeta && zip.open_new_file(fs::NAME_SAVE_META);
        if (openMeta)
        {
            zip.write(&saveMeta, SIZE_SAVE_META);
            zip.close_current_file();
        }
        auto scopedMount = create_scoped_mount(saveInfo);
        fs::copy_directory_to_zip(fs::DEFAULT_SAVE_ROOT, zip, task);
    }
    else
    {
        if (hasValidMeta)
        {
            const fslib::Path saveMetaPath{target / fs::NAME_SAVE_META};
            fslib::File saveMetaFile{saveMetaPath, FsOpenMode_Create | FsOpenMode_Write, SIZE_SAVE_META};
            if (saveMetaFile.is_open()) { saveMetaFile.write(&saveMeta, SIZE_SAVE_META); }
        }
        auto scopedMount = create_scoped_mount(saveInfo);
        fs::copy_directory(fs::DEFAULT_SAVE_ROOT, target, task);
    }

    spawningState->refresh();
    if (killTask) { task->finished(); }
}

void tasks::backup::create_new_backup_remote(sys::ProgressTask *task,
                                             data::User *user,
                                             data::TitleInfo *titleInfo,
                                             std::string remoteName,
                                             BackupMenuState *spawningState,
                                             bool killTask)
{
    remote::Storage *remote = remote::get_remote_storage();

    if (error::is_null(task) || error::is_null(user) || error::is_null(titleInfo) || error::is_null(spawningState) ||
        error::is_null(remote))
    {
        return;
    }

    const uint64_t applicationID   = titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(applicationID);
    if (error::is_null(saveInfo))
    {
        task->finished();
        return;
    }

    const fslib::Path tempPath{STRING_JKSV_TEMP};
    const int popTicks            = ui::PopMessageManager::DEFAULT_TICKS;
    const char *uploadTemplate    = strings::get_by_name(strings::names::IO_STATUSES, 5);
    const char *popErrorDeleting  = strings::get_by_name(strings::names::BACKUPMENU_POPS, 4);
    const char *popErrorCreating  = strings::get_by_name(strings::names::BACKUPMENU_POPS, 5);
    const char *popMetaFailed     = strings::get_by_name(strings::names::BACKUPMENU_POPS, 8);
    const char *popErrorUploading = strings::get_by_name(strings::names::BACKUPMENU_POPS, 10);

    fs::MiniZip zip{tempPath};
    if (!zip.is_open())
    {
        ui::PopMessageManager::push_message(popTicks, popErrorCreating);
        task->finished();
        return;
    }

    fs::SaveMetaData saveMeta{};
    const bool hasMeta     = fs::fill_save_meta_data(saveInfo, saveMeta);
    const bool metaOpened  = hasMeta && zip.open_new_file(fs::NAME_SAVE_META);
    const bool metaWritten = metaOpened && zip.write(&saveMeta, SIZE_SAVE_META);
    const bool metaClosed  = metaWritten && zip.close_current_file();
    if (hasMeta && (!metaOpened || !metaWritten || !metaClosed)) // This isn't fatal.
    {
        ui::PopMessageManager::push_message(popTicks, popMetaFailed);
    }

    {
        auto scopedMount = create_scoped_mount(saveInfo);
        fs::copy_directory_to_zip(fs::DEFAULT_SAVE_ROOT, zip, task);
    }
    zip.close();

    {
        const std::string status = stringutil::get_formatted_string(uploadTemplate, remoteName.data());
        task->set_status(status);
    }
    const bool uploaded    = remote->upload_file(tempPath, remoteName, task);
    const bool deleteError = error::fslib(fslib::delete_file(tempPath));
    if (!uploaded || deleteError) { ui::PopMessageManager::push_message(popTicks, popErrorUploading); }

    spawningState->refresh();
    if (killTask) { task->finished(); }
}

void tasks::backup::overwrite_backup_local(sys::ProgressTask *task, BackupMenuState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    data::User *user               = taskData->user;
    data::TitleInfo *titleInfo     = taskData->titleInfo;
    const fslib::Path &target      = taskData->path;
    BackupMenuState *spawningState = taskData->spawningState;
    const int popTicks             = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popErrorDeleting   = strings::get_by_name(strings::names::BACKUPMENU_POPS, 4);

    const bool isDirectory = fslib::directory_exists(target);
    const bool dirFailed   = isDirectory && error::fslib(fslib::delete_directory_recursively(target));
    const bool fileFailed  = !isDirectory && error::fslib(fslib::delete_file(target));
    if (dirFailed && fileFailed)
    {
        ui::PopMessageManager::push_message(popTicks, popErrorDeleting);
        task->finished();
        return;
    }
    tasks::backup::create_new_backup_local(task, user, titleInfo, target, spawningState);
}

void tasks::backup::overwrite_backup_remote(sys::ProgressTask *task, BackupMenuState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    remote::Storage *remote        = remote::get_remote_storage();
    data::User *user               = taskData->user;
    data::TitleInfo *titleInfo     = taskData->titleInfo;
    const uint64_t applicationID   = titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(applicationID);
    remote::Item *target           = taskData->remoteItem;
    if (error::is_null(remote) || error::is_null(saveInfo) || error::is_null(target))
    {
        task->finished();
        return;
    }

    const fslib::Path tempPath{STRING_JKSV_TEMP};
    const int popTicks           = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popErrorDeleting = strings::get_by_name(strings::names::BACKUPMENU_POPS, 4);
    const char *popErrorMeta     = strings::get_by_name(strings::names::BACKUPMENU_POPS, 8);
    const char *statusUploading  = strings::get_by_name(strings::names::IO_STATUSES, 5);

    fs::MiniZip zip{tempPath};
    if (!zip.is_open())
    {
        task->finished();
        return;
    }

    // This is repetitive but there's no other way to really accomplish this.
    fs::SaveMetaData saveMeta{};
    const bool hasMeta     = fs::fill_save_meta_data(saveInfo, saveMeta);
    const bool metaOpened  = hasMeta && zip.open_new_file(fs::NAME_SAVE_META);
    const bool metaWritten = metaOpened && zip.write(&saveMeta, SIZE_SAVE_META);
    const bool metaClosed  = metaWritten && zip.close_current_file();
    if (hasMeta && (!metaOpened || !metaWritten || !metaClosed))
    {
        ui::PopMessageManager::push_message(popTicks, popErrorMeta);
    }

    {
        auto scopedMount = create_scoped_mount(saveInfo);
        fs::copy_directory_to_zip(fs::DEFAULT_SAVE_ROOT, zip, task);
    }
    zip.close();

    {
        const char *targetName   = target->get_name().data();
        const std::string status = stringutil::get_formatted_string(statusUploading, targetName);
        task->set_status(status);
    }
    remote->patch_file(target, tempPath, task);

    const bool deleteError = error::fslib(fslib::delete_file(tempPath));
    if (deleteError) { ui::PopMessageManager::push_message(popTicks, popErrorDeleting); };

    task->finished();
}

void tasks::backup::restore_backup_local(sys::ProgressTask *task, BackupMenuState::TaskData taskData)
{
    static constexpr size_t SIZE_META = sizeof(fs::SaveMetaData);
    if (error::is_null(task)) { return; }

    remote::Storage *remote        = remote::get_remote_storage();
    data::User *user               = taskData->user;
    data::TitleInfo *titleInfo     = taskData->titleInfo;
    const fslib::Path &target      = taskData->path;
    BackupMenuState *spawningState = taskData->spawningState;
    if (error::is_null(remote) || error::is_null(user) || error::is_null(titleInfo) || error::is_null(spawningState))
    {
        task->finished();
        return;
    }

    const uint64_t applicationID   = titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(applicationID);
    const uint8_t saveType         = user->get_account_save_type();
    const uint64_t journalSize     = titleInfo->get_journal_size(saveType);
    if (error::is_null(saveInfo))
    {
        task->finished();
        return;
    }

    const bool autoBackup = config::get_by_key(config::keys::AUTO_BACKUP_ON_RESTORE);
    const bool autoUpload = config::get_by_key(config::keys::AUTO_UPLOAD);
    const bool exportZip  = autoUpload || config::get_by_key(config::keys::EXPORT_TO_ZIP);
    const bool isDir      = fslib::directory_exists(target);
    const bool hasZipExt  = std::strstr(target.full_path(), STRING_ZIP_EXT);

    const int popTicks            = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popErrorResetting = strings::get_by_name(strings::names::BACKUPMENU_POPS, 2);
    const char *popErrorCreating  = strings::get_by_name(strings::names::BACKUPMENU_POPS, 5);
    const char *popErrorOpenZip   = strings::get_by_name(strings::names::EXTRASMENU_POPS, 7);
    const char *statusUploading   = strings::get_by_name(strings::names::IO_STATUSES, 5);

    if (autoBackup)
    {
        fslib::Path autoTarget{};
        const size_t lastSlash = target.find_last_of('/');
        if (lastSlash == target.NOT_FOUND)
        {
            ui::PopMessageManager::push_message(popTicks, popErrorCreating);
            task->finished();
            return;
        }

        const char *safeNickname = user->get_path_safe_nickname();
        autoTarget = target.sub_path(lastSlash) / "AUTO - " + safeNickname + " - " + stringutil::get_date_string();
        if (exportZip) { autoTarget += ".zip"; };

        {
            auto scopedMount = create_scoped_mount(saveInfo);
            tasks::backup::create_new_backup_local(task, user, titleInfo, autoTarget, spawningState, false);
        }

        // Not sure if this is really needed here, but I'm sure someone will point it out.
        if (autoUpload && remote)
        {
            const std::string status = stringutil::get_formatted_string(statusUploading, autoTarget.get_filename());
            task->set_status(status);
            remote->upload_file(autoTarget, autoTarget.get_filename(), task);
            fslib::delete_file(autoTarget); // I don't care if this fails,
        }
    }

    const bool resetError  = error::fslib(fslib::delete_directory_recursively(fs::DEFAULT_SAVE_ROOT));
    const bool commitError = error::fslib(fslib::commit_data_to_file_system(fs::DEFAULT_SAVE_MOUNT));
    if (resetError || commitError)
    {
        ui::PopMessageManager::push_message(popTicks, popErrorResetting);
        task->finished();
        return;
    }

    if (!isDir && hasZipExt)
    {
        fs::MiniUnzip unzip{target};
        if (!unzip.is_open())
        {
            ui::PopMessageManager::push_message(popTicks, popErrorOpenZip);
            task->finished();
            return;
        }

        read_and_process_meta(unzip, taskData, task);
        auto scopedMount = create_scoped_mount(saveInfo);
        fs::copy_zip_to_directory(unzip, fs::DEFAULT_SAVE_ROOT, journalSize, fs::DEFAULT_SAVE_MOUNT, task);
    }
    else if (isDir)
    {
        read_and_process_meta(target, taskData, task);
        auto scopedMount = create_scoped_mount(saveInfo);
        fs::copy_directory_commit(target, fs::DEFAULT_SAVE_ROOT, fs::DEFAULT_SAVE_MOUNT, journalSize, task);
    }
    else
    {
        auto scopedMount = create_scoped_mount(saveInfo);
        fs::copy_file_commit(target, fs::DEFAULT_SAVE_ROOT, fs::DEFAULT_SAVE_MOUNT, journalSize, task);
    }

    spawningState->refresh();
    task->finished();
}

void tasks::backup::restore_backup_remote(sys::ProgressTask *task, BackupMenuState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    data::User *user               = taskData->user;
    data::TitleInfo *titleInfo     = taskData->titleInfo;
    const uint64_t applicationID   = titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(applicationID);
    if (error::is_null(user) || error::is_null(titleInfo) || error::is_null(saveInfo))
    {
        task->finished();
        return;
    }

    const int popTicks                 = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popErrorOpeningZip     = strings::get_by_name(strings::names::BACKUPMENU_POPS, 3);
    const char *popErrorDeleting       = strings::get_by_name(strings::names::BACKUPMENU_POPS, 4);
    const char *popErrorDownloading    = strings::get_by_name(strings::names::BACKUPMENU_POPS, 9);
    const char *popErrorProcessingMeta = strings::get_by_name(strings::names::BACKUPMENU_POPS, 11);
    const char *downloadingFormat      = strings::get_by_name(strings::names::IO_STATUSES, 4);

    remote::Storage *remote = remote::get_remote_storage();
    if (!remote)
    {
        task->finished();
        return;
    }

    remote::Item *target          = taskData->remoteItem;
    const char *statusDownloading = strings::get_by_name(strings::names::IO_STATUSES, 4);
    const fslib::Path tempPath{"sdmc:/temp.zip"};

    {
        const char *name         = target->get_name().data();
        const std::string status = stringutil::get_formatted_string(downloadingFormat, name);
        task->set_status(status);
    }

    const bool downloaded = remote->download_file(target, tempPath, task);
    if (!downloaded)
    {
        ui::PopMessageManager::push_message(popTicks, popErrorDownloading);
        task->finished();
        return;
    }

    fs::MiniUnzip backup{tempPath};
    if (!backup.is_open())
    {
        ui::PopMessageManager::push_message(popTicks, popErrorOpeningZip);
        task->finished();
        return;
    }

    const uint8_t saveType     = user->get_account_save_type();
    const uint64_t journalSize = titleInfo->get_journal_size(saveType);
    read_and_process_meta(backup, taskData, task);
    {
        fs::ScopedSaveMount saveMount{fs::DEFAULT_SAVE_MOUNT, saveInfo};
        fs::copy_zip_to_directory(backup, fs::DEFAULT_SAVE_ROOT, journalSize, fs::DEFAULT_SAVE_MOUNT, task);
    }
    backup.close();

    const bool deleteError = error::fslib(fslib::delete_file(tempPath));
    if (deleteError) { ui::PopMessageManager::push_message(popTicks, popErrorDeleting); }

    task->finished();
}

void tasks::backup::delete_backup_local(sys::Task *task, BackupMenuState::TaskData taskData)
{
    if (error::is_null(task)) { return; }
    const fslib::Path &path        = taskData->path;
    BackupMenuState *spawningState = taskData->spawningState;
    const int popTicks             = ui::PopMessageManager::DEFAULT_TICKS;
    const char *statusTemplate     = strings::get_by_name(strings::names::IO_STATUSES, 3);
    const char *popFailed          = strings::get_by_name(strings::names::BACKUPMENU_POPS, 4);

    {
        const std::string status = stringutil::get_formatted_string(statusTemplate, path.full_path());
        task->set_status(status);
    }

    const bool isDir     = fslib::directory_exists(path);
    const bool dirError  = isDir && error::fslib(fslib::delete_directory_recursively(path));
    const bool fileError = !isDir && error::fslib(fslib::delete_file(path));
    if (dirError || fileError) { ui::PopMessageManager::push_message(popTicks, popFailed); }
    spawningState->refresh();
    task->finished();
}

void tasks::backup::delete_backup_remote(sys::Task *task, BackupMenuState::TaskData taskData)
{
    remote::Storage *remote = remote::get_remote_storage();
    if (error::is_null(task) || error::is_null(remote)) { return; }

    remote::Item *target           = taskData->remoteItem;
    BackupMenuState *spawningState = taskData->spawningState;
    const int popTicks             = ui::PopMessageManager::DEFAULT_TICKS;
    const char *statusTemplate     = strings::get_by_name(strings::names::IO_STATUSES, 3);
    const char *popFailed          = strings::get_by_name(strings::names::BACKUPMENU_POPS, 4);

    {
        const std::string status = stringutil::get_formatted_string(statusTemplate, target->get_name().data());
        task->set_status(status);
    }

    const bool deleted = remote->delete_item(target);
    if (!deleted) { ui::PopMessageManager::push_message(popTicks, popFailed); }

    spawningState->refresh();
    task->finished();
}

void tasks::backup::upload_backup(sys::ProgressTask *task, BackupMenuState::TaskData taskData)
{
    remote::Storage *remote = remote::get_remote_storage();
    if (error::is_null(task) || error::is_null(remote)) { return; }

    const fslib::Path &path        = taskData->path;
    BackupMenuState *spawningState = taskData->spawningState;
    const char *statusTemplate     = strings::get_by_name(strings::names::IO_STATUSES, 5);

    {
        const char *filename     = path.get_filename();
        const std::string status = stringutil::get_formatted_string(statusTemplate, filename);
        task->set_status(status);
    }
    // The backup menu should've made sure the remote is pointing to the correct location.
    remote->upload_file(path, path.get_filename(), task);

    spawningState->refresh();
    task->finished();
}

static bool read_and_process_meta(const fslib::Path &targetDir, BackupMenuState::TaskData taskData, sys::ProgressTask *task)
{
    if (error::is_null(task)) { return false; }

    data::User *user               = taskData->user;
    data::TitleInfo *titleInfo     = taskData->titleInfo;
    const uint64_t applicationID   = titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(applicationID);
    if (error::is_null(saveInfo)) { return false; }

    const int popTicks             = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popErrorProcessing = strings::get_by_name(strings::names::BACKUPMENU_POPS, 11);
    const char *statusProcessing   = strings::get_by_name(strings::names::BACKUPMENU_STATUS, 0);

    task->set_status(statusProcessing);

    const fslib::Path metaPath{targetDir / fs::NAME_SAVE_META};
    fslib::File metaFile{metaPath, FsOpenMode_Read};
    if (!metaFile.is_open())
    {
        ui::PopMessageManager::push_message(popTicks, popErrorProcessing);
        return false;
    }

    fs::SaveMetaData metaData{};
    const bool metaRead  = metaFile.read(&metaData, SIZE_SAVE_META) == SIZE_SAVE_META;
    const bool processed = metaRead && fs::process_save_meta_data(saveInfo, metaData);
    if (!metaRead || !processed)
    {
        ui::PopMessageManager::push_message(popTicks, popErrorProcessing);
        return false;
    }

    return true;
}

static bool read_and_process_meta(fs::MiniUnzip &unzip, BackupMenuState::TaskData taskData, sys::ProgressTask *task)
{
    if (error::is_null(task)) { return false; }

    data::User *user               = taskData->user;
    data::TitleInfo *titleInfo     = taskData->titleInfo;
    const uint64_t applicationID   = titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(applicationID);
    if (error::is_null(saveInfo)) { return false; }

    const int popTicks             = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popErrorProcessing = strings::get_by_name(strings::names::BACKUPMENU_POPS, 11);
    const char *statusProcessing   = strings::get_by_name(strings::names::BACKUPMENU_STATUS, 0);

    task->set_status(statusProcessing);

    fs::SaveMetaData saveMeta{};
    const bool metaFound     = unzip.locate_file(fs::NAME_SAVE_META);
    const bool metaRead      = metaFound && unzip.read(&saveMeta, SIZE_SAVE_META) == SIZE_SAVE_META;
    const bool metaProcessed = metaRead && fs::process_save_meta_data(saveInfo, saveMeta);
    if (!metaFound || !metaRead || !metaProcessed)
    {
        ui::PopMessageManager::push_message(popTicks, popErrorProcessing);
        return false;
    }
    return true;
}

static fs::ScopedSaveMount create_scoped_mount(const FsSaveDataInfo *saveInfo)
{
    const int popTicks           = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popErrorMounting = strings::get_by_name(strings::names::BACKUPMENU_POPS, 14);
    fs::ScopedSaveMount saveMount{fs::DEFAULT_SAVE_MOUNT, saveInfo};
    if (!saveMount.is_open()) { ui::PopMessageManager::push_message(popTicks, popErrorMounting); }

    return saveMount;
}

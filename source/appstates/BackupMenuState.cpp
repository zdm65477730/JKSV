#include "appstates/BackupMenuState.hpp"

#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/ProgressState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "keyboard.hpp"
#include "logger.hpp"
#include "remote/remote.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "system/system.hpp"
#include "ui/PopMessageManager.hpp"
#include "ui/TextScroll.hpp"

#include <cstring>

namespace
{
    /// @brief This is the length allotted for naming backups.
    constexpr size_t SIZE_NAME_LENGTH = 0x80;

    /// @brief This is just so there isn't random .zip comparisons everywhere.
    const char *STRING_ZIP_EXTENSION = ".zip";

    // These make some things cleaner and easier to type.
    using TaskConfirm     = ConfirmState<sys::Task, TaskState, BackupMenuState::DataStruct>;
    using ProgressConfirm = ConfirmState<sys::ProgressTask, ProgressState, BackupMenuState::DataStruct>;
} // namespace

// Declarations here. Definitions after class.
// Create new backup in targetPath
static void create_new_backup(sys::ProgressTask *task,
                              data::User *user,
                              data::TitleInfo *titleInfo,
                              fslib::Path targetPath,
                              BackupMenuState *spawningState);
// Overwrites and existing backup.
static void overwrite_backup(sys::ProgressTask *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct);
// Restores a backup and requires confirmation to do so. Takes a shared_ptr to a TargetStruct.
static void restore_backup(sys::ProgressTask *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct);
// Deletes a backup and requires confirmation to do so. Takes a shared_ptr to a TargetStruct.
static void delete_backup(sys::Task *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct);
// Uploads a backup to the remote server.
static void upload_backup(sys::ProgressTask *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct);

BackupMenuState::BackupMenuState(data::User *user, data::TitleInfo *titleInfo)
    : m_user(user)
    , m_titleInfo(titleInfo)
    , m_saveType(m_user->get_account_save_type())
    , m_directoryPath(config::get_working_directory() / m_titleInfo->get_path_safe_title())
    , m_directoryListing(m_directoryPath)
    , m_dataStruct(std::make_shared<BackupMenuState::DataStruct>())
    , m_controlGuide(strings::get_by_name(strings::names::CONTROL_GUIDES, 2))
{
    BackupMenuState::initialize_static_members();
    BackupMenuState::ensure_target_directory();
    BackupMenuState::initialize_task_data();
    BackupMenuState::initialize_info_string();
    BackupMenuState::save_data_check();
    BackupMenuState::refresh();
}

BackupMenuState::~BackupMenuState()
{
    fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);

    sm_slidePanel->clear_elements();
    sm_slidePanel->reset();

    remote::Storage *remote = remote::get_remote_storage();
    if (remote && remote->is_initialized()) { remote->return_to_root(); }
}

void BackupMenuState::update()
{
    const bool hasFocus  = BaseState::has_focus();
    const int selected   = sm_backupMenu->get_selected();
    const bool aPressed  = input::button_pressed(HidNpadButton_A);
    const bool bPressed  = input::button_pressed(HidNpadButton_B);
    const bool xPressed  = input::button_pressed(HidNpadButton_X);
    const bool yPressed  = input::button_pressed(HidNpadButton_Y);
    const bool zrPressed = input::button_pressed(HidNpadButton_ZR);

    const bool newSelected     = selected == 0;
    const bool newBackup       = aPressed && newSelected && m_saveHasData;
    const bool overwriteBackup = aPressed && !newSelected && m_saveHasData;
    const bool restoreBackup   = yPressed && !newSelected;
    const bool deleteBackup    = xPressed && !newSelected;
    const bool uploadBackup    = zrPressed && !newSelected;
    const bool popEmpty        = aPressed && !m_saveHasData;

    if (newBackup) { BackupMenuState::name_and_create_backup(); }
    else if (overwriteBackup) { BackupMenuState::confirm_overwrite(); }
    else if (restoreBackup) { BackupMenuState::confirm_restore(); }
    else if (deleteBackup) { BackupMenuState::confirm_delete(); }
    else if (uploadBackup) { BackupMenuState::upload_backup(); }
    else if (popEmpty) { BackupMenuState::pop_save_empty(); }
    else if (bPressed) { sm_slidePanel->close(); }
    else if (sm_slidePanel->is_closed()) { BaseState::deactivate(); }

    m_titleScroll.update(hasFocus);
    sm_slidePanel->update(hasFocus);
    sm_backupMenu->update(hasFocus);
}

void BackupMenuState::render()
{
    const bool hasFocus = BaseState::has_focus();
    SDL_Texture *target = sm_slidePanel->get_target();

    sm_slidePanel->clear_target();
    m_titleScroll.render(target, hasFocus);

    sdl::render_line(target, 10, 42, sm_panelWidth - 10, 42, colors::WHITE);
    sdl::render_line(target, 10, 648, sm_panelWidth - 10, 648, colors::WHITE);
    sdl::text::render(target, 32, 673, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_controlGuide);

    sm_menuRenderTarget->clear(colors::TRANSPARENT);
    sm_backupMenu->render(sm_menuRenderTarget->get(), hasFocus);
    sm_menuRenderTarget->render(target, 0, 43);

    sm_slidePanel->render(NULL, hasFocus);
}

void BackupMenuState::refresh()
{
    m_directoryListing.open(m_directoryPath);
    if (!m_directoryListing) { return; }

    sm_backupMenu->reset();
    m_menuEntries.clear();

    sm_backupMenu->add_option(strings::get_by_name(strings::names::BACKUPMENU_MENU, 0));
    m_menuEntries.push_back({MenuEntryType::Null, 0});
    for (int64_t i = 0; i < m_directoryListing.get_count(); i++)
    {
        sm_backupMenu->add_option(m_directoryListing[i]);
        m_menuEntries.push_back({MenuEntryType::Local, static_cast<int>(i)});
    }
}

void BackupMenuState::save_data_written()
{
    if (!m_saveHasData) { m_saveHasData = true; }
}

void BackupMenuState::name_and_create_backup()
{
    const bool autoName             = config::get_by_key(config::keys::AUTO_NAME_BACKUPS);
    const bool exportZip            = config::get_by_key(config::keys::EXPORT_TO_ZIP);
    const bool autoUpload           = config::get_by_key(config::keys::AUTO_UPLOAD);
    const bool zrHeld               = input::button_held(HidNpadButton_ZR);
    const char *keyboardHeader      = strings::get_by_name(strings::names::KEYBOARD, 0);
    const bool autoNamed            = (autoName || zrHeld); // This can be eval'd here.
    char name[SIZE_NAME_LENGTH + 1] = {0};

    std::snprintf(name, SIZE_NAME_LENGTH, "%s - %s", m_user->get_path_safe_nickname(), stringutil::get_date_string().c_str());

    const bool named = autoNamed || keyboard::get_input(SwkbdType_QWERTY, name, keyboardHeader, name, SIZE_NAME_LENGTH);
    if (!named) { return; }

    fslib::Path target{m_directoryPath / name};
    const bool hasZipExt = std::strstr(target.full_path(), ".zip"); // This might not be the best check.
    if ((exportZip || autoUpload) && !hasZipExt) { target += ".zip"; }
    else if (!exportZip && !autoUpload && !hasZipExt)
    {
        const bool targetExists  = fslib::directory_exists(target);
        const bool targetCreated = !targetExists && fslib::create_directory(target);
    }
    auto newBackupTask = std::make_shared<ProgressState>(create_new_backup, m_user, m_titleInfo, target, this);
    StateManager::push_state(newBackupTask);
}

void BackupMenuState::confirm_overwrite()
{
    const int selected          = sm_backupMenu->get_selected();
    const MenuEntry &entry      = m_menuEntries.at(selected);
    const bool holdRequired     = config::get_by_key(config::keys::HOLD_FOR_OVERWRITE);
    const char *confirmTemplate = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 0);
    m_dataStruct->path          = m_directoryPath / m_directoryListing[entry.index];

    const std::string query = stringutil::get_formatted_string(confirmTemplate, m_directoryListing[entry.index]);
    auto confirm            = std::make_shared<ProgressConfirm>(query, holdRequired, overwrite_backup, m_dataStruct);
    StateManager::push_state(confirm);
}

void BackupMenuState::confirm_restore()
{
    const int selected           = sm_backupMenu->get_selected();
    const MenuEntry &entry       = m_menuEntries.at(selected);
    const int popTicks           = ui::PopMessageManager::DEFAULT_MESSAGE_TICKS;
    const bool holdRequired      = config::get_by_key(config::keys::HOLD_FOR_RESTORATION);
    const char *confirmTemplate  = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 1);
    const char *popBackupEmpty   = strings::get_by_name(strings::names::BACKUPMENU_POPS, 1);
    const char *popSysNotAllowed = strings::get_by_name(strings::names::BACKUPMENU_POPS, 6);

    const bool isSystem       = BackupMenuState::is_system_save_data();
    const bool allowSystem    = config::get_by_key(config::keys::ALLOW_WRITING_TO_SYSTEM);
    const bool isValidRestore = !isSystem || allowSystem;
    if (!isValidRestore)
    {
        ui::PopMessageManager::push_message(popTicks, popSysNotAllowed);
        return;
    }

    const fslib::Path target     = m_directoryPath / m_directoryListing[entry.index];
    const bool targetIsDirectory = fslib::directory_exists(target);
    const bool backupIsGood      = targetIsDirectory ? fs::directory_has_contents(target) : fs::zip_has_contents(target);
    if (!backupIsGood)
    {
        ui::PopMessageManager::push_message(popTicks, popBackupEmpty);
        return;
    }

    m_dataStruct->path      = target;
    const std::string query = stringutil::get_formatted_string(confirmTemplate, m_directoryListing[entry.index]);
    auto confirm            = std::make_shared<ProgressConfirm>(query, holdRequired, restore_backup, m_dataStruct);
    StateManager::push_state(confirm);
}

void BackupMenuState::confirm_delete()
{
    const int selected          = sm_backupMenu->get_selected();
    const MenuEntry &entry      = m_menuEntries.at(selected);
    const bool holdRequired     = config::get_by_key(config::keys::HOLD_FOR_DELETION);
    const char *confirmTemplate = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 2);
    m_dataStruct->path          = m_directoryPath / m_directoryListing[entry.index];

    const std::string query = stringutil::get_formatted_string(confirmTemplate, m_directoryListing[entry.index]);
    auto confirm            = std::make_shared<TaskConfirm>(query, holdRequired, delete_backup, m_dataStruct);

    StateManager::push_state(confirm);
}

void BackupMenuState::upload_backup() {}

void BackupMenuState::pop_save_empty()
{
    const int ticks      = ui::PopMessageManager::DEFAULT_MESSAGE_TICKS;
    const char *popEmpty = strings::get_by_name(strings::names::BACKUPMENU_POPS, 0);
    ui::PopMessageManager::push_message(ticks, popEmpty);
}

void BackupMenuState::initialize_static_members()
{
    constexpr int SDL_TEX_FLAGS = SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET;
    if (sm_backupMenu && sm_slidePanel && sm_menuRenderTarget && sm_panelWidth) { return; }

    sm_panelWidth       = sdl::text::get_width(22, m_controlGuide) + 64;
    sm_backupMenu       = std::make_shared<ui::Menu>(8, 8, sm_panelWidth - 14, 24, 600);
    sm_slidePanel       = std::make_unique<ui::SlideOutPanel>(sm_panelWidth, ui::SlideOutPanel::Side::Right);
    sm_menuRenderTarget = sdl::TextureManager::create_load_texture("backupMenuTarget", sm_panelWidth, 600, SDL_TEX_FLAGS);
}

void BackupMenuState::ensure_target_directory()
{
    // If this is enabled, don't bother.
    const bool autoUpload      = config::get_by_key(config::keys::AUTO_UPLOAD);
    const bool directoryNeeded = !autoUpload && !fslib::directory_exists(m_directoryPath);
    const bool directoryCreate = directoryNeeded && fslib::create_directory(m_directoryPath);
}

void BackupMenuState::initialize_task_data()
{
    m_dataStruct->user          = m_user;
    m_dataStruct->titleInfo     = m_titleInfo;
    m_dataStruct->spawningState = this;
}

void BackupMenuState::initialize_info_string()
{
    const char *nickname         = m_user->get_nickname();
    const char *title            = m_titleInfo->get_title();
    const std::string infoString = stringutil::get_formatted_string("`%s` - %s", nickname, title);
    m_titleScroll.create(infoString, 22, sm_panelWidth, 8, true, colors::WHITE);
}

void BackupMenuState::save_data_check()
{
    fslib::Directory saveRoot{fs::DEFAULT_SAVE_ROOT};
    m_saveHasData = saveRoot.get_count() > 0;
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
    if (!saveInfo)
    {
        logger::log("Error retrieving save data information for %016lX.", titleInfo->get_application_id());
        task->finished();
        return;
    }

    // I got tired of typing out the cast.
    fs::SaveMetaData saveMeta;
    bool hasMeta = fs::fill_save_meta_data(saveInfo, saveMeta);

    // This extension search is lazy and needs to be revised.
    if (config::get_by_key(config::keys::EXPORT_TO_ZIP) || std::strstr(targetPath.full_path(), "zip"))
    {
        zipFile newBackup = zipOpen64(targetPath.full_path(), APPEND_STATUS_CREATE);
        if (!newBackup)
        {
            // To do: Pop up.
            logger::log("Error opening zip for backup.");
            task->finished();
            return;
        }

        if (hasMeta)
        {
            // Data for save meta.
            zip_fileinfo saveMetaInfo;
            fs::create_zip_fileinfo(saveMetaInfo);

            // Write meta to zip.
            int zipError = zipOpenNewFileInZip64(newBackup,
                                                 fs::NAME_SAVE_META.data(),
                                                 &saveMetaInfo,
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
        }
        fs::copy_directory_to_zip(fs::DEFAULT_SAVE_ROOT, newBackup, task);
        zipClose(newBackup, NULL);
    }
    else
    {
        {
            fslib::Path saveMetaPath = targetPath / fs::NAME_SAVE_META;
            fslib::File saveMetaOut(saveMetaPath, FsOpenMode_Create | FsOpenMode_Write, sizeof(fs::SaveMetaData));
            if (saveMetaOut && hasMeta) { saveMetaOut.write(&saveMeta, sizeof(fs::SaveMetaData)); }
        }
        fs::copy_directory(fs::DEFAULT_SAVE_ROOT, targetPath, 0, {}, task);
    }

    // Refresh.
    spawningState->refresh();

    task->finished();
}

static void overwrite_backup(sys::ProgressTask *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct)
{
    // I hate typing this stuff over and over.
    static const char *STRING_ERROR_PREFIX = "Error overwriting backup: %s";

    // Might need this later.
    FsSaveDataInfo *saveInfo = dataStruct->user->get_save_info_by_id(dataStruct->titleInfo->get_application_id());

    // Wew this is a fun one to read, but it takes care of everything in one go.
    if ((fslib::directory_exists(dataStruct->path) && !fslib::delete_directory_recursively(dataStruct->path)) ||
        (fslib::file_exists(dataStruct->path) && !fslib::delete_file(dataStruct->path)))
    {
        logger::log(STRING_ERROR_PREFIX, fslib::error::get_string());
        task->finished();
        return;
    }

    // Gonna need a new save meta.
    fs::SaveMetaData meta;
    bool hasMeta = fs::fill_save_meta_data(saveInfo, meta);

    if (std::strstr(STRING_ZIP_EXTENSION, dataStruct->path.full_path()))
    {
        zipFile backupZip = zipOpen64(dataStruct->path.full_path(), APPEND_STATUS_CREATE);
        if (!backupZip)
        {
            logger::log("Error overwriting backup: Couldn't create new zip!");
            task->finished();
            return;
        }

        // Need the zip info for the meta.
        if (hasMeta)
        {
            zip_fileinfo saveMetaInfo;
            fs::create_zip_fileinfo(saveMetaInfo);

            int zipError = zipOpenNewFileInZip64(backupZip,
                                                 fs::NAME_SAVE_META.data(),
                                                 &saveMetaInfo,
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
                zipWriteInFileInZip(backupZip, &meta, sizeof(fs::SaveMetaData));
                zipCloseFileInZip(backupZip);
            }
        }

        fs::copy_directory_to_zip(fs::DEFAULT_SAVE_ROOT, backupZip, task);
        zipClose(backupZip, NULL);
    } // I hope this check works for making sure this is a folder
    else if (dataStruct->path.get_extension() == nullptr && fslib::create_directory(dataStruct->path))
    {
        // Write this quick.
        {
            fslib::Path metaPath = dataStruct->path / fs::NAME_SAVE_META;
            fslib::File metaFile(metaPath, FsOpenMode_Create | FsOpenMode_Write, sizeof(fs::SaveMetaData));
            if (metaFile && hasMeta) { metaFile.write(&meta, sizeof(fs::SaveMetaData)); }
        }

        fs::copy_directory(fs::DEFAULT_SAVE_ROOT, dataStruct->path, 0, {}, task);
    }
    task->finished();
}

static void restore_backup(sys::ProgressTask *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct)
{
    // Going to need this later.
    FsSaveDataInfo *saveInfo = dataStruct->user->get_save_info_by_id(dataStruct->titleInfo->get_application_id());
    if (!saveInfo)
    {
        // To do: Log this.
        task->finished();
        return;
    }

    // Wipe the save root first. Forgot to commit the changes before. Oops.
    if (!fslib::delete_directory_recursively(fs::DEFAULT_SAVE_ROOT) ||
        !fslib::commit_data_to_file_system(fs::DEFAULT_SAVE_MOUNT))
    {
        logger::log("Error restoring save: %s", fslib::error::get_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::BACKUPMENU_POPS, 2));
        task->finished();
        return;
    }

    if (fslib::directory_exists(dataStruct->path))
    {
        {
            // Process the save meta if it's there.
            fs::SaveMetaData meta    = {0};
            fslib::Path saveMetaPath = dataStruct->path / fs::NAME_SAVE_META;
            fslib::File saveMetaFile(saveMetaPath, FsOpenMode_Read);
            if (saveMetaFile && saveMetaFile.read(&meta, sizeof(fs::SaveMetaData)) == sizeof(fs::SaveMetaData))
            {
                // Set this so at least the user knows something is going on. Extending saves can take a decent chunk of
                // time.
                task->set_status(strings::get_by_name(strings::names::BACKUPMENU_STATUS, 0));
                fs::process_save_meta_data(saveInfo, meta);
            }
        }

        fs::copy_directory(dataStruct->path, fs::DEFAULT_SAVE_ROOT, 0, fs::DEFAULT_SAVE_MOUNT, task);
    }
    else if (std::strstr(dataStruct->path.full_path(), ".zip") != NULL)
    {
        unzFile targetZip = unzOpen64(dataStruct->path.full_path());
        if (!targetZip)
        {
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                strings::get_by_name(strings::names::BACKUPMENU_POPS, 3));
            logger::log("Error opening zip for reading.");
            task->finished();
            return;
        }

        {
            // I'm not sure if this is risky or not. Guess we'll find out...
            fs::SaveMetaData meta = {0};
            // The locate_file_in_zip should pinpoint the meta file.
            if (fs::locate_file_in_zip(targetZip, fs::NAME_SAVE_META) && unzOpenCurrentFile(targetZip) == UNZ_OK &&
                unzReadCurrentFile(targetZip, &meta, sizeof(fs::SaveMetaData)) == sizeof(fs::SaveMetaData))
            {
                task->set_status(strings::get_by_name(strings::names::BACKUPMENU_STATUS, 0));
                fs::process_save_meta_data(saveInfo, meta);
            }
        }

        fs::copy_zip_to_directory(targetZip, fs::DEFAULT_SAVE_ROOT, 0, fs::DEFAULT_SAVE_MOUNT, task);
        unzClose(targetZip);
    }
    else { fs::copy_file(dataStruct->path, fs::DEFAULT_SAVE_ROOT, 0, fs::DEFAULT_SAVE_MOUNT, task); }

    // Update this just in case.
    dataStruct->spawningState->save_data_written();

    task->finished();
}

static void delete_backup(sys::Task *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct)
{
    if (task) { task->set_status(strings::get_by_name(strings::names::IO_STATUSES, 3), dataStruct->path.full_path()); }

    if (fslib::directory_exists(dataStruct->path) && !fslib::delete_directory_recursively(dataStruct->path))
    {
        logger::log("Error deleting folder backup: %s", fslib::error::get_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::BACKUPMENU_POPS, 4));
    }
    else if (fslib::file_exists(dataStruct->path) && !fslib::delete_file(dataStruct->path))
    {
        logger::log("Error deleting backup: %s", fslib::error::get_string());
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::BACKUPMENU_POPS, 4));
    }
    dataStruct->spawningState->refresh();
    task->finished();
}

static void upload_backup(sys::ProgressTask *task, std::shared_ptr<BackupMenuState::DataStruct> dataStruct)
{
    if (task) { task->set_status(strings::get_by_name(strings::names::BACKUPMENU_STATUS, 1), dataStruct->path.get_filename()); }

    // To do: This but flashier.
    remote::Storage *remote = remote::get_remote_storage();

    remote->upload_file(dataStruct->path);

    task->finished();
}

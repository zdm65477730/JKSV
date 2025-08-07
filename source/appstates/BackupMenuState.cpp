#include "appstates/BackupMenuState.hpp"

#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/FadeState.hpp"
#include "appstates/ProgressState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "keyboard.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
#include "tasks/backup.hpp"
#include "ui/PopMessageManager.hpp"
#include "ui/TextScroll.hpp"

#include <cstring>

namespace
{
    // These make some things cleaner and easier to type.
    using TaskConfirm     = ConfirmState<sys::Task, TaskState, BackupMenuState::DataStruct>;
    using ProgressConfirm = ConfirmState<sys::ProgressTask, ProgressState, BackupMenuState::DataStruct>;
} // namespace

BackupMenuState::BackupMenuState(data::User *user, data::TitleInfo *titleInfo)
    : m_user(user)
    , m_titleInfo(titleInfo)
    , m_saveType(m_user->get_account_save_type())
    , m_directoryPath(config::get_working_directory() / m_titleInfo->get_path_safe_title())
    , m_dataStruct(std::make_shared<BackupMenuState::DataStruct>())
    , m_controlGuide(strings::get_by_name(strings::names::CONTROL_GUIDES, 2))
{
    BackupMenuState::initialize_static_members();
    BackupMenuState::ensure_target_directory();
    BackupMenuState::initialize_remote_storage();
    BackupMenuState::initialize_task_data();
    BackupMenuState::initialize_info_string();
    BackupMenuState::save_data_check();
    BackupMenuState::refresh();
}

BackupMenuState::~BackupMenuState()
{
    sm_slidePanel->clear_elements();
    sm_slidePanel->reset();
    sm_backupMenu->reset();

    remote::Storage *remote = remote::get_remote_storage();
    if (remote) { remote->return_to_root(); }
}

std::shared_ptr<BackupMenuState> BackupMenuState::create(data::User *user, data::TitleInfo *titleInfo)
{
    return std::make_shared<BackupMenuState>(user, titleInfo);
}

std::shared_ptr<BackupMenuState> BackupMenuState::create_and_push(data::User *user, data::TitleInfo *titleInfo)
{
    auto newState = BackupMenuState::create(user, titleInfo);
    StateManager::push_state(newState);
    return newState;
}

void BackupMenuState::update()
{
    const bool hasFocus = BaseState::has_focus();
    const bool isOpen   = sm_slidePanel->is_open();
    sm_slidePanel->update(hasFocus);
    if (!isOpen) { return; }

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
    sm_backupMenu->update(hasFocus);
}

void BackupMenuState::render()
{
    const bool hasFocus        = BaseState::has_focus();
    sdl::SharedTexture &target = sm_slidePanel->get_target();

    sm_slidePanel->clear_target();
    m_titleScroll.render(target, hasFocus);

    sdl::render_line(target, 10, 42, sm_panelWidth - 10, 42, colors::WHITE);
    sdl::render_line(target, 10, 648, sm_panelWidth - 10, 648, colors::WHITE);
    sdl::text::render(target, 32, 673, 22, sdl::text::NO_WRAP, colors::WHITE, m_controlGuide);

    sm_menuRenderTarget->clear(colors::TRANSPARENT);
    sm_backupMenu->render(sm_menuRenderTarget, hasFocus);
    sm_menuRenderTarget->render(target, 0, 43);

    sm_slidePanel->render(sdl::Texture::Null, hasFocus);
}

void BackupMenuState::refresh()
{
    const bool autoUpload   = config::get_by_key(config::keys::AUTO_UPLOAD);
    remote::Storage *remote = remote::get_remote_storage();
    m_directoryListing.open(m_directoryPath);
    if (!autoUpload && !m_directoryListing.is_open()) { return; }
    sm_backupMenu->reset();
    m_menuEntries.clear();

    const char *optionNew = strings::get_by_name(strings::names::BACKUPMENU_MENU, 0);
    sm_backupMenu->add_option(optionNew);
    m_menuEntries.push_back({MenuEntryType::Null, 0});

    if (remote)
    {
        const std::string_view prefix = remote->get_prefix();
        remote->get_directory_listing(m_remoteListing);
        int index{};
        for (const remote::Item *item : m_remoteListing)
        {
            const std::string_view name = item->get_name();
            const std::string option    = stringutil::get_formatted_string("%s %s", prefix.data(), name.data());

            sm_backupMenu->add_option(option);
            m_menuEntries.push_back({MenuEntryType::Remote, index++});
        }
    }

    const int64_t listingCount = m_directoryListing.get_count();
    for (int64_t i = 0; i < listingCount; i++)
    {
        sm_backupMenu->add_option(m_directoryListing[i]);
        m_menuEntries.push_back({MenuEntryType::Local, static_cast<int>(i)});
    }
}

void BackupMenuState::save_data_written()
{
    if (!m_saveHasData) { m_saveHasData = true; }
}

void BackupMenuState::initialize_static_members()
{
    if (sm_backupMenu && sm_slidePanel && sm_menuRenderTarget && sm_panelWidth) { return; }

    sm_panelWidth = sdl::text::get_width(22, m_controlGuide) + 64;
    sm_backupMenu = std::make_shared<ui::Menu>(8, 8, sm_panelWidth - 16, 24, 600);
    sm_slidePanel = std::make_unique<ui::SlideOutPanel>(sm_panelWidth, ui::SlideOutPanel::Side::Right);
    sm_menuRenderTarget =
        sdl::TextureManager::create_load_texture("backupMenuTarget", sm_panelWidth, 600, SDL_TEXTUREACCESS_TARGET);
}

void BackupMenuState::ensure_target_directory()
{
    const int popTicks    = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popFailed = strings::get_by_name(strings::names::BACKUPMENU_POPS, 12);

    // If this is enabled, don't bother.
    const bool autoUpload      = config::get_by_key(config::keys::AUTO_UPLOAD);
    const bool directoryNeeded = !autoUpload && !fslib::directory_exists(m_directoryPath);
    const bool directoryFailed = directoryNeeded && error::fslib(fslib::create_directory(m_directoryPath));
    if (!autoUpload && directoryFailed) { ui::PopMessageManager::push_message(popTicks, popFailed); }
}

void BackupMenuState::initialize_task_data()
{
    // The other members are set upon actions being taken.
    m_dataStruct->user          = m_user;
    m_dataStruct->titleInfo     = m_titleInfo;
    m_dataStruct->spawningState = this;
}

void BackupMenuState::initialize_info_string()
{
    const char *nickname         = m_user->get_nickname();
    const char *title            = m_titleInfo->get_title();
    const std::string infoString = stringutil::get_formatted_string("`%s` - %s", nickname, title);

    m_titleScroll.initialize(infoString, 8, 8, sm_panelWidth - 16, 30, 22, colors::WHITE, colors::TRANSPARENT);
}

void BackupMenuState::save_data_check()
{
    const uint64_t applicationID   = m_titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo = m_user->get_save_info_by_id(applicationID);
    if (error::is_null(saveInfo)) { return; }

    fs::ScopedSaveMount saveMount{fs::DEFAULT_SAVE_MOUNT, saveInfo};
    fslib::Directory saveRoot{fs::DEFAULT_SAVE_ROOT};
    m_saveHasData = saveRoot.is_open() && saveRoot.get_count() > 0;
}

void BackupMenuState::initialize_remote_storage()
{
    remote::Storage *remote = remote::get_remote_storage();
    if (error::is_null(remote)) { return; }

    const bool supportsUtf8            = remote->supports_utf8();
    const std::string_view remoteTitle = supportsUtf8 ? m_titleInfo->get_title() : m_titleInfo->get_path_safe_title();
    const bool remoteDirExists         = remote->directory_exists(remoteTitle);
    const bool remoteDirCreated        = !remoteDirExists && remote->create_directory(remoteTitle);
    if (!remoteDirExists && !remoteDirCreated) { return; }

    const remote::Item *remoteDir = remote->get_directory_by_name(remoteTitle);
    if (!remoteDir) { return; }

    remote->change_directory(remoteDir);
}

void BackupMenuState::name_and_create_backup()
{
    static constexpr size_t SIZE_NAME_LENGTH    = 0x80;
    static constexpr const char *STRING_ZIP_EXT = ".zip";

    remote::Storage *remote = remote::get_remote_storage();
    const bool autoName     = config::get_by_key(config::keys::AUTO_NAME_BACKUPS);
    const bool autoUpload   = config::get_by_key(config::keys::AUTO_UPLOAD);
    const bool exportZip    = autoUpload || config::get_by_key(config::keys::EXPORT_TO_ZIP);
    const bool zrHeld       = input::button_held(HidNpadButton_ZR);
    const bool autoNamed    = (autoName || zrHeld); // This can be eval'd here.

    char name[SIZE_NAME_LENGTH + 1] = {0};
    {
        const char *nickname   = m_user->get_path_safe_nickname();
        const std::string date = stringutil::get_date_string();
        std::snprintf(name, SIZE_NAME_LENGTH, "%s - %s", nickname, date.c_str());
    }

    const char *keyboardHeader = strings::get_by_name(strings::names::KEYBOARD, 0);
    const bool named = autoNamed || keyboard::get_input(SwkbdType_QWERTY, name, keyboardHeader, name, SIZE_NAME_LENGTH);
    if (!named) { return; }

    const bool hasZipExt = std::strstr(name, STRING_ZIP_EXT); // This might not be the best check.
    if (autoUpload && remote)
    {
        if (!hasZipExt) { std::strncat(name, STRING_ZIP_EXT, SIZE_NAME_LENGTH); }
        ProgressState::create_and_push(tasks::backup::create_new_backup_remote,
                                       m_user,
                                       m_titleInfo,
                                       std::string{name},
                                       this,
                                       true);
    }
    else
    {
        fslib::Path target{m_directoryPath / name};
        if (!hasZipExt && (autoUpload || exportZip)) { target += STRING_ZIP_EXT; } // We're going to append zip either way.

        ProgressState::create_and_push(tasks::backup::create_new_backup_local, m_user, m_titleInfo, target, this, true);
    }
}

void BackupMenuState::confirm_overwrite()
{
    const int selected          = sm_backupMenu->get_selected();
    const MenuEntry &entry      = m_menuEntries.at(selected);
    const bool holdRequired     = config::get_by_key(config::keys::HOLD_FOR_OVERWRITE);
    const char *confirmTemplate = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 0);

    if (entry.type == MenuEntryType::Remote)
    {
        m_dataStruct->remoteItem = m_remoteListing.at(entry.index);
        const char *itemName     = m_dataStruct->remoteItem->get_name().data();
        const std::string query  = stringutil::get_formatted_string(confirmTemplate, itemName);
        ProgressConfirm::create_push_fade(query, holdRequired, tasks::backup::overwrite_backup_remote, m_dataStruct);
    }
    else if (entry.type == MenuEntryType::Local)
    {
        m_dataStruct->path      = m_directoryPath / m_directoryListing[entry.index];
        const char *targetName  = m_directoryListing[entry.index];
        const std::string query = stringutil::get_formatted_string(confirmTemplate, targetName);
        ProgressConfirm::create_push_fade(query, holdRequired, tasks::backup::overwrite_backup_local, m_dataStruct);
    }
}

void BackupMenuState::confirm_restore()
{
    const int selected     = sm_backupMenu->get_selected();
    const MenuEntry &entry = m_menuEntries.at(selected);

    const int popTicks          = ui::PopMessageManager::DEFAULT_TICKS;
    const bool holdRequired     = config::get_by_key(config::keys::HOLD_FOR_RESTORATION);
    const char *confirmTemplate = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 1);

    const bool isSystem       = BackupMenuState::user_is_system();
    const bool allowSystem    = config::get_by_key(config::keys::ALLOW_WRITING_TO_SYSTEM);
    const bool isValidRestore = !isSystem || allowSystem;
    if (!isValidRestore)
    {
        const char *popSysNotAllowed = strings::get_by_name(strings::names::BACKUPMENU_POPS, 6);
        ui::PopMessageManager::push_message(popTicks, popSysNotAllowed);
        return;
    }

    if (entry.type == MenuEntryType::Local)
    {
        const char *popBackupEmpty = strings::get_by_name(strings::names::BACKUPMENU_POPS, 1);

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

        ProgressConfirm::create_push_fade(query, holdRequired, tasks::backup::restore_backup_local, m_dataStruct);
    }
    else if (entry.type == MenuEntryType::Remote)
    {
        remote::Item *target     = m_remoteListing[entry.index];
        const std::string query  = stringutil::get_formatted_string(confirmTemplate, target->get_name().data());
        m_dataStruct->remoteItem = target;

        ProgressConfirm::create_push_fade(query, holdRequired, tasks::backup::restore_backup_remote, m_dataStruct);
    }
}

void BackupMenuState::confirm_delete()
{
    const int selected          = sm_backupMenu->get_selected();
    const MenuEntry &entry      = m_menuEntries.at(selected);
    const bool holdRequired     = config::get_by_key(config::keys::HOLD_FOR_DELETION);
    const char *confirmTemplate = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 2);

    std::shared_ptr<TaskConfirm> confirm{};
    if (entry.type == MenuEntryType::Local)
    {
        m_dataStruct->path      = m_directoryPath / m_directoryListing[entry.index];
        const char *targetName  = m_directoryListing[entry.index];
        const std::string query = stringutil::get_formatted_string(confirmTemplate, targetName);

        TaskConfirm::create_and_push(query, holdRequired, tasks::backup::delete_backup_local, m_dataStruct);
    }
    else if (entry.type == MenuEntryType::Remote)
    {
        m_dataStruct->remoteItem = m_remoteListing.at(entry.index);
        const char *itemName     = m_dataStruct->remoteItem->get_name().data();
        const std::string query  = stringutil::get_formatted_string(confirmTemplate, itemName);

        TaskConfirm::create_and_push(query, holdRequired, tasks::backup::delete_backup_remote, m_dataStruct);
    }
}

void BackupMenuState::upload_backup()
{
    remote::Storage *remote = remote::get_remote_storage();
    if (error::is_null(remote)) { return; }

    const int selected     = sm_backupMenu->get_selected();
    const int popTicks     = ui::PopMessageManager::DEFAULT_TICKS;
    const MenuEntry &entry = m_menuEntries[selected];
    if (entry.type != BackupMenuState::MenuEntryType::Local) { return; }

    const char *targetName = m_directoryListing[entry.index];
    fslib::Path target     = m_directoryPath / targetName;
    const bool isDir       = fslib::directory_exists(target);
    if (isDir)
    {
        const char *popNotZip = strings::get_by_name(strings::names::BACKUPMENU_POPS, 13);
        ui::PopMessageManager::push_message(popTicks, popNotZip);
        return;
    }

    m_dataStruct->path              = std::move(target);
    const std::string_view itemName = m_dataStruct->path.get_filename();
    const bool exists               = remote->file_exists(itemName);
    if (exists)
    {
        remote::Item *remoteItem = remote->get_file_by_name(itemName);
        const char *queryFormat  = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 0);
        const std::string query  = stringutil::get_formatted_string(queryFormat, itemName.data());
        const bool holdRequired  = config::get_by_key(config::keys::HOLD_FOR_OVERWRITE);
        m_dataStruct->remoteItem = remoteItem;

        ProgressConfirm::create_and_push(query, holdRequired, tasks::backup::patch_backup, m_dataStruct);
    }
    else { ProgressState::create_and_push(tasks::backup::upload_backup, m_dataStruct); }
}

void BackupMenuState::pop_save_empty()
{
    const int ticks      = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popEmpty = strings::get_by_name(strings::names::BACKUPMENU_POPS, 0);
    ui::PopMessageManager::push_message(ticks, popEmpty);
}

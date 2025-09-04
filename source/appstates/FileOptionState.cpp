#include "appstates/FileOptionState.hpp"

#include "appstates/ConfirmState.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/TaskState.hpp"
#include "config/config.hpp"
#include "error.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "keyboard.hpp"
#include "logging/logger.hpp"
#include "mathutil.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "tasks/fileoptions.hpp"

namespace
{
    enum
    {
        COPY,
        DELETE,
        RENAME,
        CREATE_DIR,
        PROPERTIES,
        CLOSE
    };

    // These make things easier to read and type.
    using TaskConfirm     = ConfirmState<sys::Task, TaskState, FileOptionState::DataStruct>;
    using ProgressConfirm = ConfirmState<sys::ProgressTask, ProgressState, FileOptionState::DataStruct>;
}

FileOptionState::FileOptionState(FileModeState *spawningState)
    : m_spawningState(spawningState)
    , m_dataStruct(std::make_shared<FileOptionState::DataStruct>())
{
    FileOptionState::initialize_static_members();
    FileOptionState::set_menu_side();
    FileOptionState::initialize_data_struct();
}

void FileOptionState::update()
{
    const bool hasFocus = BaseState::has_focus();

    FileOptionState::update_x_coord();
    if (!m_inPlace) { return; }

    if (m_updateSource)
    {
        FileOptionState::update_filemode_source();
        m_updateSource = false;
    }
    else if (m_updateDest)
    {
        FileOptionState::update_filemode_dest();
        m_updateDest = false;
    }

    sm_copyMenu->update(hasFocus);
    const int selected  = sm_copyMenu->get_selected();
    const bool aPressed = input::button_pressed(HidNpadButton_A);
    const bool bPressed = input::button_pressed(HidNpadButton_B);

    if (aPressed)
    {
        switch (selected)
        {
            case COPY:   FileOptionState::copy_target(); break;
            case DELETE: FileOptionState::delete_target(); break;
            case RENAME: FileOptionState::rename_target(); break;
        }
    }
    else if (bPressed) { FileOptionState::close(); }
    else if (FileOptionState::is_closed()) { FileOptionState::deactivate_state(); }
}

void FileOptionState::render()
{
    const bool hasFocus = BaseState::has_focus();

    sm_dialog->render(sdl::Texture::Null, hasFocus);
    sm_copyMenu->render(sdl::Texture::Null, hasFocus);

    // I just didn't like this disappearing.
    m_spawningState->render_control_guide();
}

void FileOptionState::update_source() { m_updateSource = true; }

void FileOptionState::update_destination() { m_updateDest = true; }

void FileOptionState::initialize_static_members()
{
    if (sm_copyMenu && sm_dialog) { return; }

    sm_copyMenu = ui::Menu::create(0, 236, 234, 20, 720); // High target height is a workaround.
    sm_dialog   = ui::DialogBox::create(520, 218, 256, 256);

    // This never changes, so...
    for (int i = 0; const char *menuOption = strings::get_by_name(strings::names::FILEOPTION_MENU, i); i++)
    {
        sm_copyMenu->add_option(menuOption);
    }
}

void FileOptionState::set_menu_side()
{
    const bool target = m_spawningState->m_target;
    m_x               = target ? 1280 : -240;

    m_targetX = target ? 840 : 200;
    sm_dialog->set_x(m_x);
    sm_copyMenu->set_x(m_x + 9);
}

void FileOptionState::initialize_data_struct() { m_dataStruct->spawningState = this; }

void FileOptionState::update_filemode_source()
{
    const fslib::Path &sourcePath = m_spawningState->get_source_path();
    fslib::Directory &sourceDir   = m_spawningState->get_source_directory();
    ui::Menu &sourceMenu          = m_spawningState->get_source_menu();

    m_spawningState->initialize_directory_menu(sourcePath, sourceDir, sourceMenu);
}

void FileOptionState::update_filemode_dest()
{
    const fslib::Path &destPath = m_spawningState->get_destination_path();
    fslib::Directory &destDir   = m_spawningState->get_destination_directory();
    ui::Menu &destMenu          = m_spawningState->get_destination_menu();

    m_spawningState->initialize_directory_menu(destPath, destDir, destMenu);
}

void FileOptionState::update_x_coord()
{
    if (m_x == m_targetX) { return; }

    // We're going to borrow the scaling from the FileMode
    const int add = (m_targetX - m_x) / m_spawningState->m_scaling;
    m_x += add;

    const int distance = math::Util<int>::absolute_distance(m_x, m_targetX);
    if (distance <= 4)
    {
        m_x       = m_targetX;
        m_inPlace = true;
    }
    sm_dialog->set_x(m_x);
    sm_copyMenu->set_x(m_x + 9);
}

void FileOptionState::copy_target()
{
    const int64_t journalSize = m_spawningState->m_journalSize;

    const fslib::Path &sourcePath     = m_spawningState->get_source_path();
    const fslib::Directory &sourceDir = m_spawningState->get_source_directory();
    const ui::Menu &sourceMenu        = m_spawningState->get_source_menu();

    const fslib::Path &destPath     = m_spawningState->get_destination_path();
    const fslib::Directory &destDir = m_spawningState->get_destination_directory();
    const ui::Menu &destMenu        = m_spawningState->get_destination_menu();

    const int sourceSelected = sourceMenu.get_selected();
    const int destSelected   = destMenu.get_selected();

    const int sourceIndex = sourceSelected - 2;
    const int destIndex   = destSelected - 2;

    fslib::Path fullSource{sourcePath};
    if (sourceSelected > 1)
    {
        const int dirIndex = sourceSelected - 2;
        fullSource /= sourceDir[sourceIndex];
    }

    fslib::Path fullDest{destPath};
    if (destSelected == 0) { fullDest /= sourceDir[sourceIndex]; }
    if (destSelected > 1)
    {
        fullDest /= destDir[destIndex];
        if (sourceSelected > 1) { fullDest /= sourceDir[sourceIndex]; }
    }

    // Reminder: JK, you move these. That's why the string is blank if they're declared past this point.
    const std::string sourceString = fullSource.string();
    const std::string destString   = fullDest.string();
    m_dataStruct->sourcePath       = std::move(fullSource);
    m_dataStruct->destPath         = std::move(fullDest);
    m_dataStruct->journalSize      = journalSize;

    const char *copyFormat  = strings::get_by_name(strings::names::FILEOPTION_CONFS, 0);
    const std::string query = stringutil::get_formatted_string(copyFormat, sourceString.c_str(), destString.c_str());

    ProgressConfirm::create_push_fade(query, false, tasks::fileoptions::copy_source_to_destination, m_dataStruct);
}

void FileOptionState::delete_target()
{
    const fslib::Path &targetPath     = m_spawningState->get_source_path();
    const fslib::Directory &targetDir = m_spawningState->get_source_directory();
    const ui::Menu &targetMenu        = m_spawningState->get_source_menu();

    fslib::Path fullTarget{targetPath};
    const int selected = targetMenu.get_selected();
    if (selected > 1)
    {
        const int dirIndex = selected - 2;
        fullTarget /= targetDir[dirIndex];
    }

    const char *deleteFormat = strings::get_by_name(strings::names::FILEOPTION_CONFS, 1);
    const std::string query  = stringutil::get_formatted_string(deleteFormat, fullTarget.string().c_str());

    m_dataStruct->sourcePath  = std::move(fullTarget);
    m_dataStruct->journalSize = m_spawningState->m_journalSize;

    TaskConfirm::create_push_fade(query, true, tasks::fileoptions::delete_target, m_dataStruct);
}

void FileOptionState::rename_target()
{
    const int popTicks            = ui::PopMessageManager::DEFAULT_TICKS;
    const fslib::Path &targetPath = m_spawningState->get_source_path();
    fslib::Directory &targetDir   = m_spawningState->get_source_directory();
    ui::Menu &targetMenu          = m_spawningState->get_source_menu();
    const int selected            = targetMenu.get_selected();
    if (selected < 2) { return; }

    char nameBuffer[FS_MAX_PATH]     = {0};
    const int dirIndex               = selected - 2;
    const char *filename             = targetDir[dirIndex].get_filename();
    const char *keyboardFormat       = strings::get_by_name(strings::names::KEYBOARD, 9);
    const std::string keyboardHeader = stringutil::get_formatted_string(keyboardFormat, filename);
    const bool validInput            = keyboard::get_input(SwkbdType_QWERTY, filename, keyboardHeader, nameBuffer, FS_MAX_PATH);
    if (validInput) { return; }

    const fslib::Path oldPath{targetPath / filename};
    const fslib::Path newPath{targetPath / nameBuffer};

    const bool isDir      = fslib::directory_exists(oldPath);
    const bool renameDir  = isDir && error::fslib(fslib::rename_directory(oldPath, newPath));
    const bool renameFile = !isDir && error::fslib(fslib::rename_file(oldPath, newPath));
    if (!renameDir && !renameFile)
    {
        const char *popFormat = strings::get_by_name(strings::names::FILEMODE_POPS, 5);
        const std::string pop = stringutil::get_formatted_string(popFormat, filename);
        ui::PopMessageManager::push_message(popTicks, pop);
    }
    else
    {
        const char *popFormat = strings::get_by_name(strings::names::FILEMODE_POPS, 4);
        const std::string pop = stringutil::get_formatted_string(popFormat, filename, nameBuffer);
        ui::PopMessageManager::push_message(popTicks, pop);
    }

    m_spawningState->initialize_directory_menu(targetPath, targetDir, targetMenu);
}

void FileOptionState::create_directory()
{
    const int popTicks            = ui::PopMessageManager::DEFAULT_TICKS;
    const fslib::Path &targetPath = m_spawningState->get_source_path();
    fslib::Directory &targetDir   = m_spawningState->get_source_directory();
    ui::Menu &targetMenu          = m_spawningState->get_source_menu();

    char nameBuffer[FS_MAX_PATH] = {0};
    const char *keyboardHeader   = strings::get_by_name(strings::names::KEYBOARD, 6);
    const bool validInput        = keyboard::get_input(SwkbdType_QWERTY, {}, keyboardHeader, nameBuffer, FS_MAX_PATH);
    if (!validInput) { return; }

    const fslib::Path fullTarget{targetPath / nameBuffer};
    const bool createError = error::fslib(fslib::create_directory(fullTarget));
    if (createError)
    {
        const char *popFormat = strings::get_by_name(strings::names::FILEMODE_POPS, 7);
        const std::string pop = stringutil::get_formatted_string(popFormat, nameBuffer);
        ui::PopMessageManager::push_message(popTicks, pop);
    }
    else
    {
        const char *popFormat = strings::get_by_name(strings::names::FILEMODE_POPS, 6);
        const std::string pop = stringutil::get_formatted_string(popFormat, nameBuffer);
        ui::PopMessageManager::push_message(popTicks, pop);
    }

    m_spawningState->initialize_directory_menu(targetPath, targetDir, targetMenu);
}

void FileOptionState::get_show_target_properties()
{
    const fslib::Path &targetPath     = m_spawningState->get_source_path();
    const fslib::Directory &targetDir = m_spawningState->get_source_directory();
    const ui::Menu &targeMenu         = m_spawningState->get_source_menu();
}

void FileOptionState::close()
{
    const bool target = m_spawningState->m_target;

    m_close   = true;
    m_targetX = target ? 1280 : -240;
}

bool FileOptionState::is_closed() { return m_close && m_x == m_targetX; }

void FileOptionState::deactivate_state()
{
    sm_copyMenu->set_selected(0);
    BaseState::deactivate();
}
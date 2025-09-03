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
    , m_commitData(m_spawningState->commit_required())
    , m_scaling(config::get_animation_scaling())
    , m_dataStruct(std::make_shared<FileOptionState::DataStruct>())
{
    FileOptionState::initialize_static_members();
    FileOptionState::set_menu_side();
}

void FileOptionState::update()
{
    const bool hasFocus = BaseState::has_focus();

    FileOptionState::update_x_coord();
    if (!m_inPlace) { return; }

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
    const bool target = m_spawningState->get_target();
    m_x               = target ? 1280 : -240;

    m_targetX = target ? 840 : 200;
    sm_dialog->set_x(m_x);
    sm_copyMenu->set_x(m_x + 9);
}

void FileOptionState::update_x_coord()
{
    if (m_x == m_targetX) { return; }

    const int add = (m_targetX - m_x) / m_scaling;
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
    fslib::Path source = m_spawningState->get_source();
    fslib::Path dest   = m_spawningState->get_destination();

    const char *copyFormat  = strings::get_by_name(strings::names::FILEOPTION_CONFS, 0);
    const std::string query = stringutil::get_formatted_string(copyFormat, source.string().c_str(), dest.string().c_str());

    m_dataStruct->sourcePath  = std::move(source);
    m_dataStruct->destPath    = std::move(dest);
    m_dataStruct->journalSize = m_journalSize;

    ProgressConfirm::create_push_fade(query, false, tasks::fileoptions::copy_source_to_destination, m_dataStruct);
}

void FileOptionState::delete_target()
{
    fslib::Path source = m_spawningState->get_source();

    const char *deleteFormat = strings::get_by_name(strings::names::FILEOPTION_CONFS, 1);
    const std::string query  = stringutil::get_formatted_string(deleteFormat, source.string().c_str());

    m_dataStruct->sourcePath  = std::move(source);
    m_dataStruct->journalSize = m_journalSize;

    TaskConfirm::create_push_fade(query, true, tasks::fileoptions::delete_target, m_dataStruct);
}

void FileOptionState::rename_target()
{
    const int popTicks           = ui::PopMessageManager::DEFAULT_TICKS;
    const bool target            = m_spawningState->get_target();
    const fslib::Path targetPath = target ? m_spawningState->get_destination() : m_spawningState->get_source();

    char nameBuffer[FS_MAX_PATH]     = {0};
    const char *filename             = targetPath.get_filename();
    const char *keyboardFormat       = strings::get_by_name(strings::names::KEYBOARD, 9);
    const std::string keyboardHeader = stringutil::get_formatted_string(keyboardFormat, filename);
    const bool validInput            = keyboard::get_input(SwkbdType_QWERTY, filename, keyboardHeader, nameBuffer, FS_MAX_PATH);
    if (validInput) { return; }

    size_t folderBegin = targetPath.find_last_of('/');
    if (folderBegin == targetPath.NOT_FOUND) { return; }
    else if (folderBegin < 1) { folderBegin = 1; }

    fslib::Path newPath{targetPath.sub_path(folderBegin) / nameBuffer};

    const bool isDir      = fslib::directory_exists(targetPath);
    const bool renameDir  = isDir && error::fslib(fslib::rename_directory(targetPath, newPath));
    const bool renameFile = !isDir && error::fslib(fslib::rename_file(targetPath, newPath));
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
}

void FileOptionState::close()
{
    const bool target = m_spawningState->get_target();

    m_close   = true;
    m_targetX = target ? 1280 : -240;
}

bool FileOptionState::is_closed() { return m_close && m_x == m_targetX; }

void FileOptionState::deactivate_state()
{
    sm_copyMenu->set_selected(0);
    BaseState::deactivate();
}
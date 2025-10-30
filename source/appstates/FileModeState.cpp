#include "appstates/FileModeState.hpp"

#include "appstates/FileOptionState.hpp"
#include "config/config.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "logging/logger.hpp"
#include "mathutil.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"

#include <cmath>

//                      ---- Construction ----

FileModeState::FileModeState(std::string_view mountA, std::string_view mountB, int64_t journalSize, bool isSystem)
    : m_mountA(mountA)
    , m_mountB(mountB)
    , m_journalSize(journalSize)
    , m_transition(15, 720, 0, 0, 15, 90, 0, 0, ui::Transition::DEFAULT_THRESHOLD)
    , m_isSystem(isSystem)
    , m_allowSystem(config::get_by_key(config::keys::ALLOW_WRITING_TO_SYSTEM))
{
    FileModeState::initialize_static_members();
    FileModeState::initialize_paths();
    FileModeState::initialize_menus();
}

//                      ---- Public functions ----

void FileModeState::update()
{
    m_transition.update();
    if (!m_transition.in_place())
    {
        const int y = m_transition.get_y();
        sm_frame->set_y(y);
        return;
    }

    const bool hasFocus = BaseState::has_focus();

    ui::Menu &menu              = FileModeState::get_source_menu();
    fslib::Path &path           = FileModeState::get_source_path();
    fslib::Directory &directory = FileModeState::get_source_directory();

    const bool aPressed     = input::button_pressed(HidNpadButton_A);
    const bool bPressed     = input::button_pressed(HidNpadButton_B);
    const bool xPressed     = input::button_pressed(HidNpadButton_X);
    const bool zlZRPressed  = input::button_pressed(HidNpadButton_ZL) || input::button_pressed(HidNpadButton_ZR);
    const bool minusPressed = input::button_pressed(HidNpadButton_Minus);

    if (aPressed) { FileModeState::enter_selected(path, directory, menu); }
    else if (bPressed) { FileModeState::up_one_directory(path, directory, menu); }
    else if (xPressed) { FileModeState::open_option_menu(directory, menu); }
    else if (zlZRPressed) { FileModeState::change_target(); }
    else if (minusPressed) { FileModeState::hide_dialog(); }
    else if (FileModeState::is_hidden()) { FileModeState::deactivate_state(); }

    menu.update(hasFocus);
    sm_controlGuide->update(hasFocus);
}

void FileModeState::render()
{
    const bool hasFocus = BaseState::has_focus();

    sm_renderTarget->clear(colors::TRANSPARENT);

    // This is here so it's rendered underneath the pop-up frame.
    sm_controlGuide->render(sdl::Texture::Null, hasFocus);

    sdl::render_line(sm_renderTarget, 617, 0, 617, 538, colors::WHITE);
    sdl::render_line(sm_renderTarget, 618, 0, 618, 538, colors::DIALOG_DARK);

    m_dirMenuA->render(sm_renderTarget, hasFocus && m_target == false);
    m_dirMenuB->render(sm_renderTarget, hasFocus && m_target);

    sm_frame->render(sdl::Texture::Null, true);

    const int y = m_transition.get_y();
    sm_renderTarget->render(sdl::Texture::Null, 23, y + 12);
}

//                      ---- Private functions ----

void FileModeState::initialize_static_members()
{
    static constexpr std::string_view RENDER_TARGET_NAME = "FMRenderTarget";

    if (sm_frame && sm_renderTarget && sm_controlGuide) { return; }

    sm_frame        = ui::Frame::create(15, 720, 1250, 555);
    sm_renderTarget = sdl::TextureManager::load(RENDER_TARGET_NAME, 1234, 538, SDL_TEXTUREACCESS_TARGET);
    sm_controlGuide = ui::ControlGuide::create(strings::get_by_name(strings::names::CONTROL_GUIDES, 4));
}

void FileModeState::initialize_paths()
{
    m_pathA = m_mountA + ":/";
    m_pathB = m_mountB + ":/";
}

void FileModeState::initialize_menus()
{
    m_dirMenuA = ui::Menu::create(8, 5, 594, 20, 538);
    m_dirMenuB = ui::Menu::create(630, 5, 594, 20, 538);

    FileModeState::initialize_directory_menu(m_pathA, m_dirA, *m_dirMenuA.get());
    FileModeState::initialize_directory_menu(m_pathB, m_dirB, *m_dirMenuB.get());
}

void FileModeState::initialize_directory_menu(const fslib::Path &path, fslib::Directory &directory, ui::Menu &menu)
{
    static constexpr const char *DIR_PREFIX  = "[D] ";
    static constexpr const char *FILE_PREFIX = "[F] ";

    directory.open(path);
    if (!directory.is_open()) { return; }

    menu.reset();
    menu.add_option(".");
    menu.add_option("..");

    for (const fslib::DirectoryEntry &entry : directory)
    {
        std::string option{};
        if (entry.is_directory()) { option = DIR_PREFIX; }
        else { option = FILE_PREFIX; }

        option += entry.get_filename();
        menu.add_option(option);
    }
}

void FileModeState::hide_dialog() noexcept
{
    if (!m_transition.in_place()) { return; }
    sm_controlGuide->reset();
    m_transition.set_target_y(720);
    m_close = true;
}

bool FileModeState::is_hidden() noexcept { return m_close && m_transition.in_place(); }

void FileModeState::enter_selected(fslib::Path &path, fslib::Directory &directory, ui::Menu &menu)
{
    const int selected = menu.get_selected();

    switch (selected)
    {
        case 0: return;
        case 1: FileModeState::up_one_directory(path, directory, menu); break;
        default:
        {
            const int dirIndex                 = selected - 2;
            const fslib::DirectoryEntry &entry = directory[dirIndex];
            if (entry.is_directory()) { FileModeState::enter_directory(path, directory, menu, entry); }
        }
        break;
    }
}

void FileModeState::open_option_menu(fslib::Directory &directory, ui::Menu &menu)
{
    const int selected = menu.get_selected();

    if (selected == 0 || selected > 1) { FileOptionState::create_and_push(this); }
}

void FileModeState::change_target() { m_target = m_target ? false : true; }

void FileModeState::up_one_directory(fslib::Path &path, fslib::Directory &directory, ui::Menu &menu)
{
    if (FileModeState::path_is_root(path)) { return; }

    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == path.NOT_FOUND) { return; }
    else if (lastSlash <= 0) { lastSlash = 1; }

    fslib::Path newPath{path.sub_path(lastSlash)};
    path = std::move(newPath);
    FileModeState::initialize_directory_menu(path, directory, menu);
}

void FileModeState::enter_directory(fslib::Path &path,
                                    fslib::Directory &directory,
                                    ui::Menu &menu,
                                    const fslib::DirectoryEntry &entry)
{
    if (!entry.is_directory()) { return; }

    path /= entry.get_filename();
    FileModeState::initialize_directory_menu(path, directory, menu);
}

ui::Menu &FileModeState::get_source_menu() noexcept { return m_target ? *m_dirMenuB.get() : *m_dirMenuA.get(); }

ui::Menu &FileModeState::get_destination_menu() noexcept { return m_target ? *m_dirMenuA.get() : *m_dirMenuB.get(); }

fslib::Path &FileModeState::get_source_path() noexcept { return m_target ? m_pathB : m_pathA; }

fslib::Path &FileModeState::get_destination_path() noexcept { return m_target ? m_pathA : m_pathB; }

fslib::Directory &FileModeState::get_source_directory() noexcept { return m_target ? m_dirB : m_dirA; }

fslib::Directory &FileModeState::get_destination_directory() noexcept { return m_target ? m_dirA : m_dirB; }

void FileModeState::deactivate_state() noexcept
{
    sm_frame->set_y(720);
    fslib::close_file_system(m_mountA);
    fslib::close_file_system(m_mountB);
    BaseState::deactivate();
}

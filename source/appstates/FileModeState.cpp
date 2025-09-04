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

FileModeState::FileModeState(std::string_view mountA, std::string_view mountB, int64_t journalSize)
    : m_mountA(mountA)
    , m_mountB(mountB)
    , m_journalSize(journalSize)
    , m_y(720.f)
    , m_targetY(91.0f)
    , m_scaling(config::get_animation_scaling())
{
    FileModeState::initialize_static_members();
    FileModeState::initialize_paths();
    FileModeState::initialize_menus();
}

void FileModeState::update()
{
    FileModeState::update_y_coord();
    if (!m_inPlace) { return; }

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
}

void FileModeState::render()
{
    const bool hasFocus = BaseState::has_focus();

    sm_renderTarget->clear(colors::TRANSPARENT);

    // This is here so it's rendered underneath the pop-up frame.
    if (hasFocus) { FileModeState::render_control_guide(); }

    sdl::render_line(sm_renderTarget, 617, 0, 617, 538, colors::WHITE);
    sdl::render_line(sm_renderTarget, 618, 0, 618, 538, colors::DIALOG_DARK);

    m_dirMenuA->render(sm_renderTarget, hasFocus && m_target == false);
    m_dirMenuB->render(sm_renderTarget, hasFocus && m_target);

    sm_frame->render(sdl::Texture::Null, true);
    sm_renderTarget->render(sdl::Texture::Null, 23, m_y + 12);
}

void FileModeState::initialize_static_members()
{
    static constexpr std::string_view RENDER_TARGET_NAME = "FMRenderTarget";

    static constexpr int CONTROL_GUIDE_START_X = 1220;

    if (sm_frame && sm_renderTarget) { return; }

    sm_frame         = ui::Frame::create(15, 720, 1250, 555);
    sm_renderTarget  = sdl::TextureManager::load(RENDER_TARGET_NAME, 1234, 538, SDL_TEXTUREACCESS_TARGET);
    sm_controlGuide  = strings::get_by_name(strings::names::CONTROL_GUIDES, 4);
    sm_controlGuideX = CONTROL_GUIDE_START_X - sdl::text::get_width(22, sm_controlGuide);
}

void FileModeState::initialize_paths()
{
    const std::string pathA = m_mountA + ":/";
    const std::string pathB = m_mountB + ":/";

    m_pathA = pathA;
    m_pathB = pathB;
}

void FileModeState::initialize_menus()
{
    m_dirMenuA = ui::Menu::create(8, 8, 594, 22, 538);
    m_dirMenuB = ui::Menu::create(626, 8, 594, 22, 538);

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

void FileModeState::update_y_coord() noexcept
{
    if (m_y == m_targetY) { return; }

    const double add      = (m_targetY - m_y) / m_scaling;
    const double distance = math::Util<double>::absolute_distance(m_targetY, m_y);
    m_y += std::round(add);

    // The second condition is a fix for when scaling is 1.
    if (distance <= 4 || m_y == m_targetY)
    {
        m_y       = m_targetY;
        m_inPlace = true;
    }

    sm_frame->set_y(m_y);
}

void FileModeState::hide_dialog() noexcept
{
    if (!m_inPlace) { return; }
    m_targetY = 720;
}

bool FileModeState::is_hidden() noexcept { return m_inPlace && m_targetY == 720; }

void FileModeState::enter_selected(fslib::Path &path, fslib::Directory &directory, ui::Menu &menu)
{
    const int selected = menu.get_selected();

    if (selected == 1) { FileModeState::up_one_directory(path, directory, menu); }
    else
    {
        const int dirIndex                 = selected - 2;
        const fslib::DirectoryEntry &entry = directory[dirIndex];
        if (entry.is_directory()) { FileModeState::enter_directory(path, directory, menu, entry); }
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

void FileModeState::render_control_guide()
{
    sdl::text::render(sdl::Texture::Null, sm_controlGuideX, 673, 22, sdl::text::NO_WRAP, colors::WHITE, sm_controlGuide);
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
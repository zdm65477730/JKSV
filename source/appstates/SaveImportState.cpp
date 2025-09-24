#include "appstates/SaveImportState.hpp"

#include "config/config.hpp"
#include "input.hpp"

SaveImportState::SaveImportState(data::User *user)
    : m_user(user)
    , m_saveDir()
{
    SaveImportState::initialize_static_members();
    SaveImportState::initialize_directory_menu();
}

void SaveImportState::update()
{
    const bool hasFocus = BaseState::has_focus();

    sm_slidePanel->update(hasFocus);
    if (!sm_slidePanel->is_open()) { return; }

    const bool aPressed = input::button_pressed(HidNpadButton_A);
    const bool bPressed = input::button_pressed(HidNpadButton_B);

    if (aPressed) {}
    else if (bPressed) { sm_slidePanel->close(); }
    else if (sm_slidePanel->is_closed()) { SaveImportState::deactivate_state(); }
}

void SaveImportState::render()
{
    const bool hasFocus = BaseState::has_focus();

    sm_slidePanel->clear_target();
    sm_slidePanel->render(sdl::Texture::Null, hasFocus);
}

void SaveImportState::initialize_static_members()
{
    static constexpr int SIZE_PANEL_WIDTH = 512;

    if (sm_saveMenu && sm_slidePanel) { return; }

    sm_saveMenu   = ui::Menu::create(8, 8, 492, 22, 720);
    sm_slidePanel = ui::SlideOutPanel::create(SIZE_PANEL_WIDTH, ui::SlideOutPanel::Side::Right);

    sm_slidePanel->push_new_element(sm_saveMenu);
}

void SaveImportState::initialize_directory_menu()
{
    const fslib::Path saveDirPath{config::get_working_directory() / "saves"};

    m_saveDir.open(saveDirPath);
    if (!m_saveDir.is_open()) { return; }

    for (const fslib::DirectoryEntry &entry : m_saveDir) { sm_saveMenu->add_option(entry.get_filename()); }
}

void SaveImportState::deactivate_state()
{
    sm_saveMenu->reset();
    sm_slidePanel->reset();
    BaseState::deactivate();
}

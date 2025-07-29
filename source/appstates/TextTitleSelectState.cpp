#include "appstates/TextTitleSelectState.hpp"

#include "StateManager.hpp"
#include "appstates/BackupMenuState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TitleOptionState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "fs/save_mount.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "sdl.hpp"

#include <string_view>

namespace
{
    // All of these states share this same target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

TextTitleSelectState::TextTitleSelectState(data::User *user)
    : TitleSelectCommon{}
    , m_user{user}
    , m_titleSelectMenu{32, 8, 1000, 20, 555}
    , m_renderTarget{sdl::TextureManager::create_load_texture(SECONDARY_TARGET,
                                                              1080,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)}
{
    TextTitleSelectState::refresh();
}

void TextTitleSelectState::update()
{
    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input::button_pressed(HidNpadButton_A);
    const bool bPressed = input::button_pressed(HidNpadButton_B);
    const bool xPressed = input::button_pressed(HidNpadButton_X);
    const bool yPressed = input::button_pressed(HidNpadButton_Y);

    m_titleSelectMenu.update(BaseState::has_focus());

    if (aPressed) { TextTitleSelectState::create_backup_menu(); }
    else if (xPressed) { TextTitleSelectState::create_title_option_menu(); }
    else if (yPressed) { TextTitleSelectState::add_remove_favorite(); }
    else if (bPressed) { BaseState::deactivate(); }
}

void TextTitleSelectState::render()
{
    m_renderTarget->clear(colors::TRANSPARENT);
    m_titleSelectMenu.render(m_renderTarget->get(), BaseState::has_focus());
    TitleSelectCommon::render_control_guide();
    m_renderTarget->render(NULL, 201, 91);
}

void TextTitleSelectState::refresh()
{
    m_titleSelectMenu.reset();
    for (size_t i = 0; i < m_user->get_total_data_entries(); i++)
    {
        std::string option;
        uint64_t applicationID = m_user->get_application_id_at(i);
        const char *title      = data::get_title_info_by_id(applicationID)->get_title();
        if (config::is_favorite(applicationID)) { option = std::string("^\uE017^ ") + title; }
        else { option = title; }
        m_titleSelectMenu.add_option(option.c_str());
    }
}

void TextTitleSelectState::create_backup_menu()
{
    const int selected             = m_titleSelectMenu.get_selected();
    const uint64_t applicationID   = m_user->get_application_id_at(selected);
    const FsSaveDataInfo *saveInfo = m_user->get_save_info_by_id(applicationID);
    data::TitleInfo *titleInfo     = data::get_title_info_by_id(applicationID);

    auto backupMenuState = std::make_shared<BackupMenuState>(m_user, titleInfo);
    StateManager::push_state(backupMenuState);
}

void TextTitleSelectState::create_title_option_menu()
{
    const int selected           = m_titleSelectMenu.get_selected();
    const uint64_t applicationID = m_user->get_application_id_at(selected);
    data::TitleInfo *titleInfo   = data::get_title_info_by_id(applicationID);

    auto titleOptionState = std::make_shared<TitleOptionState>(m_user, titleInfo, this);
    StateManager::push_state(titleOptionState);
}

void TextTitleSelectState::add_remove_favorite()
{
    const int selected           = m_titleSelectMenu.get_selected();
    const uint64_t applicationID = m_user->get_application_id_at(selected);

    config::add_remove_favorite(applicationID);

    // This applies to all users.
    data::UserList list{};
    data::get_users(list);
    for (data::User *user : list) { user->sort_data(); }

    MainMenuState::refresh_view_states();
}

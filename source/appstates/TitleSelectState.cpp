#include "appstates/TitleSelectState.hpp"

#include "StateManager.hpp"
#include "appstates/BackupMenuState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TitleOptionState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "sdl.hpp"
#include "strings.hpp"

#include <string_view>

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

TitleSelectState::TitleSelectState(data::User *user)
    : TitleSelectCommon()
    , m_user(user)
    , m_renderTarget(sdl::TextureManager::create_load_texture(SECONDARY_TARGET,
                                                              1080,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET))
    , m_titleView(m_user) {};

std::shared_ptr<TitleSelectState> TitleSelectState::create(data::User *user)
{
    return std::make_shared<TitleSelectState>(user);
}

std::shared_ptr<TitleSelectState> TitleSelectState::create_and_push(data::User *user)
{
    auto newState = TitleSelectState::create(user);
    StateManager::push_state(newState);
    return newState;
}

void TitleSelectState::update()
{
    if (!TitleSelectState::title_count_check()) { return; }

    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input::button_pressed(HidNpadButton_A);
    const bool bPressed = input::button_pressed(HidNpadButton_B);
    const bool xPressed = input::button_pressed(HidNpadButton_X);
    const bool yPressed = input::button_pressed(HidNpadButton_Y);

    if (aPressed) { TitleSelectState::create_backup_menu(); }
    else if (xPressed) { TitleSelectState::create_title_option_menu(); }
    else if (yPressed) { TitleSelectState::add_remove_favorite(); }
    else if (bPressed) { TitleSelectState::deactivate_state(); }

    m_titleView.update(hasFocus);
}

void TitleSelectState::render()
{
    const bool hasFocus = BaseState::has_focus();

    m_renderTarget->clear(colors::TRANSPARENT);
    m_titleView.render(m_renderTarget->get(), hasFocus);
    TitleSelectCommon::render_control_guide();
    m_renderTarget->render(NULL, 201, 91);
}

void TitleSelectState::refresh() { m_titleView.refresh(); }

bool TitleSelectState::title_count_check()
{
    const int titleCount = m_user->get_total_data_entries();

    if (titleCount <= 0)
    {
        BaseState::deactivate();
        return false;
    }
    return true;
}

void TitleSelectState::create_backup_menu()
{
    const int selected           = m_titleView.get_selected();
    const uint64_t applicationID = m_user->get_application_id_at(selected);
    data::TitleInfo *titleInfo   = data::get_title_info_by_id(applicationID);

    auto backupMenu = std::make_shared<BackupMenuState>(m_user, titleInfo);
    StateManager::push_state(backupMenu);
}

void TitleSelectState::create_title_option_menu()
{
    const int selected           = m_titleView.get_selected();
    const uint64_t applicationID = m_user->get_application_id_at(selected);
    data::TitleInfo *titleInfo   = data::get_title_info_by_id(applicationID);

    auto titleOptions = std::make_shared<TitleOptionState>(m_user, titleInfo, this);
    StateManager::push_state(titleOptions);
}

void TitleSelectState::deactivate_state()
{
    m_titleView.reset();
    BaseState::deactivate();
}

void TitleSelectState::add_remove_favorite()
{
    const int selected           = m_titleView.get_selected();
    const uint64_t applicationID = m_user->get_application_id_at(selected);

    config::add_remove_favorite(applicationID);

    // This applies to all users.
    data::UserList userList;
    data::get_users(userList);
    for (data::User *user : userList) { user->sort_data(); }

    MainMenuState::refresh_view_states();
}

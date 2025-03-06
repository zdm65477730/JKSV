#include "appstates/MainMenuState.hpp"
#include "JKSV.hpp"
#include "appstates/ExtrasMenuState.hpp"
#include "appstates/SettingsState.hpp"
#include "appstates/TextTitleSelectState.hpp"
#include "appstates/TitleSelectCommon.hpp"
#include "appstates/TitleSelectState.hpp"
#include "appstates/UserOptionState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "sdl.hpp"
#include "strings.hpp"

MainMenuState::MainMenuState(void)
    : m_renderTarget(sdl::TextureManager::create_load_texture("MainMenuTarget",
                                                              200,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_background(
          sdl::TextureManager::create_load_texture("MainMenuBackground", "romfs:/Textures/MenuBackground.png")),
      m_mainMenu(50, 15, 555), m_controlGuide(strings::get_by_name(strings::names::CONTROL_GUIDES, 0)),
      m_controlGuideX(1220 - sdl::text::get_width(22, m_controlGuide))
{
    // Fetch user list.
    data::get_users(sm_users);

    // Loop through add user's icon to menu and create states.
    for (size_t i = 0; i < sm_users.size(); i++)
    {
        m_mainMenu.add_option(sm_users.at(i)->get_shared_icon());

        if (config::get_by_key(config::keys::JKSM_TEXT_MODE))
        {
            sm_states.push_back(std::make_shared<TextTitleSelectState>(sm_users.at(i)));
        }
        else
        {
            sm_states.push_back(std::make_shared<TitleSelectState>(sm_users.at(i)));
        }
    }
    // Add the settings and extras.
    sm_states.push_back(std::make_shared<SettingsState>());
    sm_states.push_back(std::make_shared<ExtrasMenuState>());

    // Create icons for the other two.
    m_settingsIcon = sdl::TextureManager::create_load_texture("SettingsIcon", "romfs:/Textures/SettingsIcon.png");
    m_extrasIcon = sdl::TextureManager::create_load_texture("ExtrasIcon", "romfs:/Textures/ExtrasIcon.png");

    // Finally add them to the end.
    m_mainMenu.add_option(m_settingsIcon);
    m_mainMenu.add_option(m_extrasIcon);
}

void MainMenuState::update(void)
{
    m_mainMenu.update(AppState::has_focus());

    int selected = m_mainMenu.get_selected();

    if (input::button_pressed(HidNpadButton_A) && selected < static_cast<int>(sm_users.size()) &&
        sm_users.at(selected)->get_total_data_entries() > 0)
    {
        sm_states.at(selected)->reactivate();
        JKSV::push_state(sm_states.at(selected));
    }
    else if (input::button_pressed(HidNpadButton_A) && selected >= static_cast<int>(sm_users.size()))
    {
        sm_states.at(selected)->reactivate();
        JKSV::push_state(sm_states.at(selected));
    }
    else if (input::button_pressed(HidNpadButton_X) && selected < static_cast<int>(sm_users.size()))
    {
        // Get pointers to data the user option state needs.
        data::User *targetUser = sm_users.at(selected);
        TitleSelectCommon *targetTitleSelect = reinterpret_cast<TitleSelectCommon *>(sm_states.at(selected).get());

        JKSV::push_state(std::make_shared<UserOptionState>(targetUser, targetTitleSelect));
    }
}

void MainMenuState::render(void)
{
    // Clear render target by rendering background to it.
    m_background->render(m_renderTarget->get(), 0, 0);
    // render menu.
    m_mainMenu.render(m_renderTarget->get(), AppState::has_focus());
    // render target to screen.
    m_renderTarget->render(NULL, 0, 91);

    // render next state for current user and control guide if this state has focus.
    if (AppState::has_focus())
    {
        sm_states.at(m_mainMenu.get_selected())->render();
        sdl::text::render(NULL, m_controlGuideX, 673, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_controlGuide);
    }
}

void MainMenuState::refresh_view_states(void)
{
    for (size_t i = 0; i < sm_users.size(); i++)
    {
        sm_users.at(i)->sort_data();
        std::static_pointer_cast<TitleSelectCommon>(sm_states.at(i))->refresh();
    }
}

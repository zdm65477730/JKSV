#include "appstates/MainMenuState.hpp"
#include "StateManager.hpp"
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

MainMenuState::MainMenuState()
    : m_renderTarget(sdl::TextureManager::create_load_texture("mainMenuTarget",
                                                              200,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_background(sdl::TextureManager::create_load_texture("mainBackground", "romfs:/Textures/MenuBackground.png")),
      m_settingsIcon(sdl::TextureManager::create_load_texture("settingsIcon", "romfs:/Textures/SettingsIcon.png")),
      m_extrasIcon(sdl::TextureManager::create_load_texture("extrasIcon", "romfs:/Textures/ExtrasIcon.png")),
      m_mainMenu(50, 15, 555), m_controlGuide(strings::get_by_name(strings::names::CONTROL_GUIDES, 0)),
      m_controlGuideX(1220 - sdl::text::get_width(22, m_controlGuide))
{
    if (!sm_settingsState || !sm_extrasState)
    {
        sm_settingsState = std::make_shared<SettingsState>();
        sm_extrasState = std::make_shared<ExtrasMenuState>();
    }

    // Grab the users and setup the main menu. Users shouldn't change.
    data::get_users(sm_users);

    for (data::User *user : sm_users)
    {
        m_mainMenu.add_option(user->get_shared_icon());
    }

    // Add the last two.
    m_mainMenu.add_option(m_settingsIcon);
    m_mainMenu.add_option(m_extrasIcon);

    // Just call this.
    MainMenuState::initialize_view_states();
}

void MainMenuState::update()
{
    const bool hasFocus = BaseState::has_focus();

    // Update the main menu.
    m_mainMenu.update(hasFocus);

    int selected = m_mainMenu.get_selected();

    // To do: Simplify this logic.
    if (input::button_pressed(HidNpadButton_A) && selected < static_cast<int>(sm_users.size()) &&
        sm_users.at(selected)->get_total_data_entries() > 0)
    {
        sm_states.at(selected)->reactivate();
        StateManager::push_state(sm_states.at(selected));
    }
    else if (input::button_pressed(HidNpadButton_A) && selected >= static_cast<int>(sm_users.size()))
    {
        sm_states.at(selected)->reactivate();
        StateManager::push_state(sm_states.at(selected));
    }
    else if (input::button_pressed(HidNpadButton_X) && selected < static_cast<int>(sm_users.size()))
    {
        // Get pointers to data the user option state needs.
        data::User *targetUser = sm_users.at(selected);
        TitleSelectCommon *targetTitleSelect = static_cast<TitleSelectCommon *>(sm_states.at(selected).get());

        StateManager::push_state(std::make_shared<UserOptionState>(targetUser, targetTitleSelect));
    }
}

void MainMenuState::render()
{
    const bool hasFocus = BaseState::has_focus();

    // Clear render target by rendering background to it.
    m_background->render(m_renderTarget->get(), 0, 0);
    // render menu.
    m_mainMenu.render(m_renderTarget->get(), hasFocus);
    // render target to screen.
    m_renderTarget->render(NULL, 0, 91);

    // render next state for current user and control guide if this state has focus. To do: Maybe this different?
    if (hasFocus)
    {
        sm_states.at(m_mainMenu.get_selected())->render();
        sdl::text::render(NULL, m_controlGuideX, 673, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_controlGuide);
    }
}

void MainMenuState::initialize_view_states()
{
    // Constructor should have taken care of the menu and user list.
    // Start by clearing the vector.
    sm_states.clear();

    // Grab this from config instead of calling it every loop.
    bool textMode = config::get_by_key(config::keys::JKSM_TEXT_MODE);

    // Loop through users.
    for (data::User *user : sm_users)
    {
        if (textMode)
        {
            sm_states.push_back(std::make_shared<TextTitleSelectState>(user));
        }
        else
        {
            sm_states.push_back(std::make_shared<TitleSelectState>(user));
        }
    }
    sm_states.push_back(sm_settingsState);
    sm_states.push_back(sm_extrasState);
}

void MainMenuState::refresh_view_states()
{
    // For this, we're only looping through the user states to be extra careful because the last two don't have the refresh() function.
    int userCount = sm_users.size();
    for (int i = 0; i < userCount; i++)
    {
        static_cast<TitleSelectCommon *>(sm_states.at(i).get())->refresh();
    }
}

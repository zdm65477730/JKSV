#include "appstates/MainMenuState.hpp"

#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
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
#include "stringutil.hpp"
#include "tasks/mainmenu.hpp"
#include "ui/PopMessageManager.hpp"

namespace
{
    using ProgressConfirm = ConfirmState<sys::ProgressTask, ProgressState, MainMenuState::DataStruct>;
}

MainMenuState::MainMenuState()
    : m_renderTarget{sdl::TextureManager::create_load_texture("mainMenuTarget",
                                                              200,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)}
    , m_background{sdl::TextureManager::create_load_texture("mainBackground", "romfs:/Textures/MenuBackground.png")}
    , m_settingsIcon{sdl::TextureManager::create_load_texture("settingsIcon", "romfs:/Textures/SettingsIcon.png")}
    , m_extrasIcon{sdl::TextureManager::create_load_texture("extrasIcon", "romfs:/Textures/ExtrasIcon.png")}
    , m_mainMenu{50, 15, 555}
    , m_controlGuide{strings::get_by_name(strings::names::CONTROL_GUIDES, 0)}
    , m_controlGuideX{static_cast<int>(1220 - sdl::text::get_width(22, m_controlGuide))}
    , m_dataStruct{std::make_shared<MainMenuState::DataStruct>()}
{
    MainMenuState::initialize_settings_extras();
    MainMenuState::initialize_menu();
    MainMenuState::initialize_view_states();
    MainMenuState::initialize_data_struct();
}

std::shared_ptr<MainMenuState> MainMenuState::create() { return std::make_shared<MainMenuState>(); }

std::shared_ptr<MainMenuState> MainMenuState::create_and_push()
{
    auto newState = MainMenuState::create();
    StateManager::push_state(newState);
    return newState;
}

void MainMenuState::update()
{
    const int selected  = m_mainMenu.get_selected();
    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input::button_pressed(HidNpadButton_A);
    const bool xPressed = input::button_pressed(HidNpadButton_X);
    const bool yPressed = input::button_pressed(HidNpadButton_Y);

    const bool toUserOptions = xPressed && selected < sm_userCount;

    if (aPressed) { MainMenuState::push_target_state(); }
    else if (toUserOptions) { MainMenuState::create_user_options(); }
    else if (yPressed) { MainMenuState::backup_all_for_all(); }

    m_mainMenu.update(hasFocus);
}

void MainMenuState::render()
{
    const bool hasFocus = BaseState::has_focus();
    const int selected  = m_mainMenu.get_selected();

    m_background->render(m_renderTarget->get(), 0, 0);
    m_mainMenu.render(m_renderTarget->get(), hasFocus);
    m_renderTarget->render(NULL, 0, 91);

    if (hasFocus)
    {
        BaseState *target = sm_states.at(selected).get();
        target->render();

        sdl::text::render(NULL, m_controlGuideX, 673, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_controlGuide);
    }
}

void MainMenuState::initialize_view_states()
{
    const bool jksmMode = config::get_by_key(config::keys::JKSM_TEXT_MODE);

    sm_states.clear();
    for (data::User *user : sm_users)
    {
        std::shared_ptr<BaseState> state{};
        if (jksmMode) { state = std::make_shared<TextTitleSelectState>(user); }
        else { state = std::make_shared<TitleSelectState>(user); }
        sm_states.push_back(state);
    }
    sm_states.push_back(sm_settingsState);
    sm_states.push_back(sm_extrasState);
}

void MainMenuState::refresh_view_states()
{
    for (int i = 0; i < sm_userCount; i++)
    {
        TitleSelectCommon *target = static_cast<TitleSelectCommon *>(sm_states.at(i).get());
        target->refresh();
    }
}

void MainMenuState::initialize_settings_extras()
{
    if (!sm_settingsState || !sm_extrasState)
    {
        sm_settingsState = std::make_shared<SettingsState>();
        sm_extrasState   = std::make_shared<ExtrasMenuState>();
    }
}

void MainMenuState::initialize_menu()
{
    data::get_users(sm_users);
    sm_userCount = sm_users.size();
    for (data::User *user : sm_users) { m_mainMenu.add_option(user->get_shared_icon()); }
    m_mainMenu.add_option(m_settingsIcon);
    m_mainMenu.add_option(m_extrasIcon);
}

void MainMenuState::initialize_data_struct() { m_dataStruct->userList = sm_users; }

void MainMenuState::push_target_state()
{
    const int popTicks          = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popNoSaveFormat = strings::get_by_name(strings::names::MAINMENU_POPS, 0);

    const int selected  = m_mainMenu.get_selected();
    const int userCount = sm_users.size();
    if (selected < userCount)
    {
        const data::User *user = sm_users[selected];
        const int titleCount   = user->get_total_data_entries();
        if (titleCount <= 0)
        {
            const char *nickname       = user->get_nickname();
            const std::string popError = stringutil::get_formatted_string(popNoSaveFormat, nickname);
            ui::PopMessageManager::push_message(popTicks, popError);
            return;
        }
    }

    auto &target = sm_states[selected];
    target->reactivate();
    StateManager::push_state(target);
}

void MainMenuState::create_user_options()
{
    const int selected             = m_mainMenu.get_selected();
    data::User *user               = sm_users.at(selected);
    TitleSelectCommon *titleSelect = static_cast<TitleSelectCommon *>(sm_states[selected].get());

    UserOptionState::create_and_push(user, titleSelect);
}

void MainMenuState::backup_all_for_all()
{
    const char *query = strings::get_by_name(strings::names::MAINMENU_CONFS, 0);
    ProgressConfirm::create_and_push(query, true, tasks::mainmenu::backup_all_for_all_users, m_dataStruct);
}

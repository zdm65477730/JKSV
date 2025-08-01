#include "appstates/ExtrasMenuState.hpp"

#include "appstates/MainMenuState.hpp"
#include "colors.hpp"
#include "data/data.hpp"
#include "input.hpp"
#include "keyboard.hpp"
#include "strings.hpp"
#include "ui/PopMessageManager.hpp"

#include <string_view>

namespace
{
    // This target is shared be a lot of states.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";

    // Enum for switch case readability.
    enum
    {
        REINIT_DATA,
        SD_TO_SD_BROWSER,
        BIS_PRODINFO_F,
        BIS_SAFE,
        BIS_SYSTEM,
        BIS_USER,
        TERMINATE_PROCESS
    };
} // namespace

ExtrasMenuState::ExtrasMenuState()
    : m_extrasMenu{32, 8, 1000, 24, 555}
    , m_renderTarget{sdl::TextureManager::create_load_texture(SECONDARY_TARGET,
                                                              1080,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)}
{
    ExtrasMenuState::initialize_menu();
}

std::shared_ptr<ExtrasMenuState> create() { return std::make_shared<ExtrasMenuState>(); }

void ExtrasMenuState::update()
{
    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input::button_pressed(HidNpadButton_A);
    const bool bPressed = input::button_pressed(HidNpadButton_B);

    m_extrasMenu.update(hasFocus);

    if (aPressed)
    {
        switch (m_extrasMenu.get_selected())
        {
            case REINIT_DATA: ExtrasMenuState::reinitialize_data(); break;
        }
    }
    else if (bPressed) { BaseState::deactivate(); }
}

void ExtrasMenuState::render()
{
    const bool hasFocus = BaseState::has_focus();

    m_renderTarget->clear(colors::TRANSPARENT);
    m_extrasMenu.render(m_renderTarget->get(), hasFocus);
    m_renderTarget->render(NULL, 201, 91);
}

void ExtrasMenuState::initialize_menu()
{
    for (int i = 0; const char *option = strings::get_by_name(strings::names::EXTRASMENU_MENU, i); i++)
    {
        m_extrasMenu.add_option(option);
    }
}

void ExtrasMenuState::reinitialize_data()
{
    const int popTicks     = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popSuccess = strings::get_by_name(strings::names::EXTRASMENU_POPS, 0);
    const char *popFailure = strings::get_by_name(strings::names::EXTRASMENU_POPS, 1);

    // Call data and make in reinit and delete the cache first.
    const bool initSuccess = data::initialize(true);
    if (!initSuccess)
    {
        ui::PopMessageManager::push_message(popTicks, popFailure);
        return;
    }

    MainMenuState::refresh_view_states();
    ui::PopMessageManager::push_message(popTicks, popSuccess);
}

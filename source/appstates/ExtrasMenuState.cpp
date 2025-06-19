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

ExtrasMenuState::ExtrasMenuState(void)
    : m_extrasMenu(32, 8, 1000, 24, 555),
      m_renderTarget(sdl::TextureManager::create_load_texture(SECONDARY_TARGET,
                                                              1080,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET))
{
    const char *extrasString = nullptr;
    int currentString = 0;
    while ((extrasString = strings::get_by_name(strings::names::EXTRAS_MENU, currentString++)) != nullptr)
    {
        m_extrasMenu.add_option(extrasString);
    }
}

void ExtrasMenuState::update(void)
{
    m_extrasMenu.update(AppState::has_focus());

    if (input::button_pressed(HidNpadButton_A))
    {
        switch (m_extrasMenu.get_selected())
        {
            case REINIT_DATA:
            {
                // Reinitialize the data with the cache cleared.
                data::initialize(true);
                ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                                    strings::get_by_name(strings::names::EXTRAS_POP_MESSAGES, 0));

                // This should be adequate for this...
                MainMenuState::refresh_view_states();
            }
            break;
        }
    }
    else if (input::button_pressed(HidNpadButton_B))
    {
        AppState::deactivate();
    }
}

void ExtrasMenuState::render(void)
{
    m_renderTarget->clear(colors::TRANSPARENT);
    m_extrasMenu.render(m_renderTarget->get(), AppState::has_focus());
    m_renderTarget->render(NULL, 201, 91);
}

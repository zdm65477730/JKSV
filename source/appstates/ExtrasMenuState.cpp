#include "appstates/ExtrasMenuState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "strings.hpp"
#include <string_view>

namespace
{
    // This target is shared be a lot of states.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

ExtrasMenuState::ExtrasMenuState(void)
    : m_extrasMenu(32, 8, 1000, 24, 555),
      m_renderTarget(sdl::TextureManager::createLoadTexture(SECONDARY_TARGET,
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

    if (input::button_pressed(HidNpadButton_B))
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

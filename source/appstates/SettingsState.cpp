#include "appstates/SettingsState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "input.hpp"
#include "stringUtil.hpp"
#include "strings.hpp"

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

static inline const char *getvalueText(uint8_t value)
{
    return value == 1 ? strings::getByName(strings::names::ON_OFF, 0) : strings::getByName(strings::names::ON_OFF, 1);
}

SettingsState::SettingsState(void)
    : m_settingsMenu(32, 8, 1000, 24, 555),
      m_renderTarget(sdl::TextureManager::createLoadTexture(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_controlGuideX(1220 - sdl::text::getWidth(22, strings::getByName(strings::names::CONTROL_GUIDES, 3)))
{
    // Add the first two, because they don't have values to display.
    m_settingsMenu.addOption(strings::getByName(strings::names::SETTINGS_MENU, 0));
    m_settingsMenu.addOption(strings::getByName(strings::names::SETTINGS_MENU, 1));

    int currentString = 2;
    const char *settingsString = nullptr;
    while (currentString < 16 && (settingsString = strings::getByName(strings::names::SETTINGS_MENU, currentString++)) != nullptr)
    {
        m_settingsMenu.addOption(stringutil::getFormattedString(settingsString, getvalueText(config::getByIndex(currentString - 1))));
    }
    // Add the scaling
    m_settingsMenu.addOption(
        stringutil::getFormattedString(strings::getByName(strings::names::SETTINGS_MENU, 17), config::getAnimationScaling()));
}

void SettingsState::update(void)
{
    m_settingsMenu.update(AppState::hasFocus());

    if (input::buttonPressed(HidNpadButton_B))
    {
        AppState::deactivate();
    }
}

void SettingsState::render(void)
{
    m_renderTarget->clear(colors::TRANSPARENT);
    m_settingsMenu.render(m_renderTarget->get(), AppState::hasFocus());
    m_renderTarget->render(NULL, 201, 91);

    if (AppState::hasFocus())
    {
        sdl::text::render(NULL,
                          m_controlGuideX,
                          673,
                          22,
                          sdl::text::NO_TEXT_WRAP,
                          colors::WHITE,
                          strings::getByName(strings::names::CONTROL_GUIDES, 3));
    }
}

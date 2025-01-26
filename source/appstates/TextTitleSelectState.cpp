#include "appstates/TextTitleSelectState.hpp"
#include "appstates/MainMenuState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include <string_view>

namespace
{
    // All of these states share this same target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

TextTitleSelectState::TextTitleSelectState(data::User *user)
    : TitleSelectCommon(), m_user(user), m_titleSelectMenu(32, 8, 1000, 20, 555),
      m_renderTarget(sdl::TextureManager::createLoadTexture(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET))
{
    TextTitleSelectState::refresh();
}

void TextTitleSelectState::update(void)
{
    m_titleSelectMenu.update(AppState::hasFocus());

    if (input::buttonPressed(HidNpadButton_Y))
    {
        config::addRemoveFavorite(m_user->getApplicationIDAt(m_titleSelectMenu.getSelected()));
        MainMenuState::refreshViewStates();
    }
    else if (input::buttonPressed(HidNpadButton_B))
    {
        AppState::deactivate();
    }
}

void TextTitleSelectState::render(void)
{
    m_renderTarget->clear(colors::TRANSPARENT);
    m_titleSelectMenu.render(m_renderTarget->get(), AppState::hasFocus());
    TitleSelectCommon::renderControlGuide();
    m_renderTarget->render(NULL, 201, 91);
}

void TextTitleSelectState::refresh(void)
{
    m_titleSelectMenu.reset();
    for (size_t i = 0; i < m_user->getTotalDataEntries(); i++)
    {
        std::string option;
        uint64_t applicationID = m_user->getApplicationIDAt(i);
        const char *title = data::getTitleInfoByID(applicationID)->getTitle();
        if (config::isFavorite(applicationID))
        {
            option = std::string("^\uE017^ ") + title;
        }
        else
        {
            option = title;
        }
        m_titleSelectMenu.addOption(option.c_str());
    }
}

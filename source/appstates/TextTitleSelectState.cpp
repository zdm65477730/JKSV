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
      m_renderTarget(sdl::TextureManager::create_load_texture(SECONDARY_TARGET,
                                                              1080,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET))
{
    TextTitleSelectState::refresh();
}

void TextTitleSelectState::update(void)
{
    m_titleSelectMenu.update(AppState::has_focus());

    if (input::button_pressed(HidNpadButton_Y))
    {
        config::add_remove_favorite(m_user->get_application_id_at(m_titleSelectMenu.get_selected()));
        MainMenuState::refresh_view_states();
    }
    else if (input::button_pressed(HidNpadButton_B))
    {
        AppState::deactivate();
    }
}

void TextTitleSelectState::render(void)
{
    m_renderTarget->clear(colors::TRANSPARENT);
    m_titleSelectMenu.render(m_renderTarget->get(), AppState::has_focus());
    TitleSelectCommon::render_control_guide();
    m_renderTarget->render(NULL, 201, 91);
}

void TextTitleSelectState::refresh(void)
{
    m_titleSelectMenu.reset();
    for (size_t i = 0; i < m_user->get_total_data_entries(); i++)
    {
        std::string option;
        uint64_t applicationID = m_user->get_application_id_at(i);
        const char *title = data::get_title_info_by_id(applicationID)->get_title();
        if (config::is_favorite(applicationID))
        {
            option = std::string("^\uE017^ ") + title;
        }
        else
        {
            option = title;
        }
        m_titleSelectMenu.add_option(option.c_str());
    }
}

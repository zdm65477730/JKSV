#include "appstates/SettingsState.hpp"
#include "appstates/MainMenuState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "data/data.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "keyboard.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include "stringutil.hpp"

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

// Declarations. Definitions after class members.
static const char *get_value_text(uint8_t value);
static const char *get_sort_type_text(uint8_t value);

SettingsState::SettingsState(void)
    : m_settingsMenu(32, 8, 1000, 24, 555),
      m_renderTarget(sdl::TextureManager::create_load_texture(SECONDARY_TARGET,
                                                              1080,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_controlGuideX(1220 - sdl::text::get_width(22, strings::get_by_name(strings::names::CONTROL_GUIDES, 3)))
{
    // Loop and allocate the strings and menu options.
    int currentString = 0;
    const char *settingsString = nullptr;
    while ((settingsString = strings::get_by_name(strings::names::SETTINGS_MENU, currentString++)) != nullptr)
    {
        m_settingsMenu.add_option(settingsString);
    }

    // Run the update routine so they're right.
    SettingsState::update_menu_options();
}

void SettingsState::update(void)
{
    m_settingsMenu.update(AppState::has_focus());

    if (input::button_pressed(HidNpadButton_A))
    {
        SettingsState::toggle_options();
    }
    else if (input::button_pressed(HidNpadButton_B))
    {
        AppState::deactivate();
    }
}

void SettingsState::render(void)
{
    m_renderTarget->clear(colors::TRANSPARENT);
    m_settingsMenu.render(m_renderTarget->get(), AppState::has_focus());
    m_renderTarget->render(NULL, 201, 91);

    if (AppState::has_focus())
    {
        sdl::text::render(NULL,
                          m_controlGuideX,
                          673,
                          22,
                          sdl::text::NO_TEXT_WRAP,
                          colors::WHITE,
                          strings::get_by_name(strings::names::CONTROL_GUIDES, 3));
    }
}

void SettingsState::update_menu_options(void)
{
    // These can be looped, so screw it.
    for (int i = 2; i < 13; i++)
    {
        std::string updatedOption =
            stringutil::get_formatted_string(strings::get_by_name(strings::names::SETTINGS_MENU, i),
                                             get_value_text(config::get_by_index(i - 2)));
        m_settingsMenu.edit_option(i, updatedOption);
    }

    // This just displays the value. The config value is offset to account for the first two settings options.
    m_settingsMenu.edit_option(13,
                               stringutil::get_formatted_string(strings::get_by_name(strings::names::SETTINGS_MENU, 13),
                                                                config::get_by_index(11)));

    // This gets the type according to the value.
    m_settingsMenu.edit_option(14,
                               stringutil::get_formatted_string(strings::get_by_name(strings::names::SETTINGS_MENU, 14),
                                                                get_sort_type_text(config::get_by_index(12))));

    // Loop again.
    for (int i = 15; i < 18; i++)
    {
        std::string updatedOption =
            stringutil::get_formatted_string(strings::get_by_name(strings::names::SETTINGS_MENU, i),
                                             get_value_text(config::get_by_index(i - 2)));
        m_settingsMenu.edit_option(i, updatedOption);
    }

    // Animating scaling.
    m_settingsMenu.edit_option(18,
                               stringutil::get_formatted_string(strings::get_by_name(strings::names::SETTINGS_MENU, 18),
                                                                config::get_animation_scaling()));
}

void SettingsState::toggle_options(void)
{
    int selected = m_settingsMenu.get_selected();


    switch (selected)
    {
        // Zip level.
        case 13:
        {
            uint8_t zipLevel = config::get_by_key(config::keys::ZIP_COMPRESSION_LEVEL);
            if (++zipLevel > 9)
            {
                zipLevel = 0;
            }
            config::set_by_key(config::keys::ZIP_COMPRESSION_LEVEL, zipLevel);
        }
        break;

        // Title sorting.
        case 14:
        {
            // Change the sorting type.
            uint8_t sortType = config::get_by_key(config::keys::TITLE_SORT_TYPE);
            if (++sortType >= 3)
            {
                sortType = 0;
            }
            config::set_by_key(config::keys::TITLE_SORT_TYPE, sortType);

            // Grab the users and resort their data.
            data::UserList list;
            data::get_users(list);
            for (data::User *user : list)
            {
                user->sort_data();
            }

            // Main the main menu refresh everything.
            MainMenuState::refresh_view_states();
        }
        break;

        // Text mode. This is handled beyond a toggle.
        case 15:
        {
            // We're gonna use this to toggle first.
            config::toggle_by_index(selected - 2);

            // Need the main state to reinit all the views.
            MainMenuState::initialize_view_states();
        }
        break;

        // UI transition scaling.
        case 18:
        {
            double scaling = config::get_animation_scaling();
            if ((scaling += 0.25f) > 4.0f)
            {
                scaling = 1.0f;
            }
            config::set_animation_scaling(scaling);
        }
        break;

        default:
        {
            config::toggle_by_index(selected - 2);
        }
        break;
    }

    // Toggle the update routine.
    SettingsState::update_menu_options();
}

static const char *get_value_text(uint8_t value)
{
    return value ? strings::get_by_name(strings::names::ON_OFF, 1) : strings::get_by_name(strings::names::ON_OFF, 0);
}

static const char *get_sort_type_text(uint8_t value)
{
    switch (value)
    {
        case 0:
        {
            return "Alphabetically";
        }
        break;

        case 1:
        {
            return "Most Played";
        }
        break;

        case 2:
        {
            return "Last Played";
        }
        break;
    }
    return nullptr;
}

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

#include <array>

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";

    // This is needed to be able to get and set keys by index. Anything "NULL" isn't a key that can be easily toggled.
    constexpr std::array<std::string_view, 19> CONFIG_KEY_ARRAY = {"NULL",
                                                                   "NULL",
                                                                   config::keys::INCLUDE_DEVICE_SAVES,
                                                                   config::keys::AUTO_BACKUP_ON_RESTORE,
                                                                   config::keys::AUTO_NAME_BACKUPS,
                                                                   config::keys::AUTO_UPLOAD,
                                                                   config::keys::HOLD_FOR_DELETION,
                                                                   config::keys::HOLD_FOR_RESTORATION,
                                                                   config::keys::HOLD_FOR_OVERWRITE,
                                                                   config::keys::ONLY_LIST_MOUNTABLE,
                                                                   config::keys::LIST_ACCOUNT_SYS_SAVES,
                                                                   config::keys::ALLOW_WRITING_TO_SYSTEM,
                                                                   config::keys::EXPORT_TO_ZIP,
                                                                   config::keys::ZIP_COMPRESSION_LEVEL,
                                                                   config::keys::TITLE_SORT_TYPE,
                                                                   config::keys::JKSM_TEXT_MODE,
                                                                   config::keys::FORCE_ENGLISH,
                                                                   config::keys::ENABLE_TRASH_BIN,
                                                                   "NULL"};
} // namespace

SettingsState::SettingsState()
    : m_settingsMenu(32, 8, 1000, 24, 555)
    , m_controlGuide(strings::get_by_name(strings::names::CONTROL_GUIDES, 3))
    , m_controlGuideX(1220 - sdl::text::get_width(22, m_controlGuide))
    , m_renderTarget(sdl::TextureManager::create_load_texture(SECONDARY_TARGET,
                                                              1080,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET))
{

    for (int i = 0; const char *setting = strings::get_by_name(strings::names::SETTINGS_MENU, i); i++)
    {
        m_settingsMenu.add_option(setting);
    }

    for (int i = 0; const char *onOff = strings::get_by_name(strings::names::ON_OFF, i); i++) { m_onOff[i] = onOff; }
    for (int i = 0; const char *sortType = strings::get_by_name(strings::names::SORT_TYPES, i); i++)
    {
        m_sortTypes[i] = sortType;
    }
    SettingsState::update_menu_options();
}

void SettingsState::update()
{
    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input::button_pressed(HidNpadButton_A);
    const bool bPressed = input::button_pressed(HidNpadButton_B);

    m_settingsMenu.update(hasFocus);
    if (aPressed) { SettingsState::toggle_options(); }
    else if (bPressed) { BaseState::deactivate(); }
}

void SettingsState::render()
{
    const bool hasFocus = BaseState::has_focus();

    m_renderTarget->clear(colors::TRANSPARENT);
    m_settingsMenu.render(m_renderTarget->get(), hasFocus);
    m_renderTarget->render(NULL, 201, 91);

    if (hasFocus) { sdl::text::render(NULL, m_controlGuideX, 673, 22, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_controlGuide); }
}

void SettingsState::update_menu_options()
{
    for (int i : {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17})
    {
        const char *optionTemplate = strings::get_by_name(strings::names::SETTINGS_MENU, i);
        const uint8_t value        = config::get_by_key(CONFIG_KEY_ARRAY[i]);
        const char *status         = SettingsState::get_status_text(value);
        const std::string option   = stringutil::get_formatted_string(optionTemplate, status);
        m_settingsMenu.edit_option(i, option);
    }

    {
        const char *zipCompTemplate = strings::get_by_name(strings::names::SETTINGS_MENU, 13);
        const uint8_t zipLevel      = config::get_by_key(CONFIG_KEY_ARRAY[13]);
        const std::string zipOption = stringutil::get_formatted_string(zipCompTemplate, zipLevel);
        m_settingsMenu.edit_option(13, zipOption);
    }

    {
        const char *titleSortTemplate    = strings::get_by_name(strings::names::SETTINGS_MENU, 14);
        const uint8_t sortType           = config::get_by_key(CONFIG_KEY_ARRAY[14]);
        const char *typeText             = SettingsState::get_sort_type_text(sortType);
        const std::string sortTypeOption = stringutil::get_formatted_string(titleSortTemplate, typeText);
        m_settingsMenu.edit_option(14, sortTypeOption);
    }

    {
        const char *scalingTemplate     = strings::get_by_name(strings::names::SETTINGS_MENU, 18);
        const double scaling            = config::get_animation_scaling();
        const std::string scalingOption = stringutil::get_formatted_string(scalingTemplate, scaling);
        m_settingsMenu.edit_option(18, scalingOption);
    }
}

void SettingsState::toggle_options()
{
    const int selected = m_settingsMenu.get_selected();
    switch (selected)
    {
        case 13: SettingsState::cycle_zip_level(); break;
        case 14: SettingsState::cycle_sort_type(); break;
        case 15: SettingsState::toggle_jksm_mode(); break;
        case 18: SettingsState::cycle_anim_scaling(); break;
        default: config::toggle_by_key(CONFIG_KEY_ARRAY[selected]);
    }
    config::save();
    SettingsState::update_menu_options();
}

void SettingsState::cycle_zip_level()
{
    uint8_t zipLevel = config::get_by_key(config::keys::ZIP_COMPRESSION_LEVEL);
    if (++zipLevel % 10 == 0) { zipLevel = 0; }
    config::set_by_key(config::keys::ZIP_COMPRESSION_LEVEL, zipLevel);
}

void SettingsState::cycle_sort_type()
{
    uint8_t sortType = config::get_by_key(config::keys::TITLE_SORT_TYPE);
    if (++sortType % 3 == 0) { sortType = 0; }
    config::set_by_key(config::keys::TITLE_SORT_TYPE, sortType);

    data::UserList users{};
    data::get_users(users);
    for (data::User *user : users) { user->sort_data(); }
    MainMenuState::refresh_view_states();
}

void SettingsState::toggle_jksm_mode()
{
    config::toggle_by_key(config::keys::JKSM_TEXT_MODE);
    MainMenuState::initialize_view_states();
}

void SettingsState::cycle_anim_scaling()
{
    double scaling = config::get_animation_scaling();
    if ((scaling += 0.25f) > 4.0f) { scaling = 1.0f; }
    config::set_animation_scaling(scaling);
}

const char *SettingsState::get_status_text(uint8_t value)
{
    if (value > 1) { return nullptr; }
    return m_onOff[value];
}

const char *SettingsState::get_sort_type_text(uint8_t value)
{
    if (value > 2) { return nullptr; }
    logger::log("return string: %s", m_sortTypes[value]);
    return m_sortTypes[value];
}

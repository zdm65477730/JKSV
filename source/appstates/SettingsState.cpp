#include "appstates/SettingsState.hpp"

#include "appstates/BlacklistEditState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/MessageState.hpp"
#include "config/config.hpp"
#include "data/data.hpp"
#include "fslib.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "keyboard.hpp"
#include "logging/error.hpp"
#include "logging/logger.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"

#include <array>

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";

    constexpr std::string_view CONFIG_KEY_NULL = "NULL";

    enum
    {
        CHANGE_WORK_DIR = 0,
        EDIT_BLACKLIST  = 1,
        CYCLE_ZIP       = 14,
        CYCLE_SORT_TYPE = 15,
        TOGGLE_JKSM     = 16,
        TOGGLE_TRASH    = 22,
        CYCLE_SCALING   = 23
    };

    // This is needed to be able to get and set keys by index. Anything "NULL" isn't a key that can be easily toggled.
    constexpr std::array<std::string_view, 24> CONFIG_KEY_ARRAY = {CONFIG_KEY_NULL,
                                                                   CONFIG_KEY_NULL,
                                                                   config::keys::INCLUDE_DEVICE_SAVES,
                                                                   config::keys::AUTO_BACKUP_ON_RESTORE,
                                                                   config::keys::AUTO_NAME_BACKUPS,
                                                                   config::keys::AUTO_UPLOAD,
                                                                   config::keys::USE_TITLE_IDS,
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
                                                                   config::keys::SHOW_DEVICE_USER,
                                                                   config::keys::SHOW_BCAT_USER,
                                                                   config::keys::SHOW_CACHE_USER,
                                                                   config::keys::SHOW_SYSTEM_USER,
                                                                   config::keys::ENABLE_TRASH_BIN,
                                                                   CONFIG_KEY_NULL};
} // namespace

SettingsState::SettingsState()
    : m_settingsMenu(ui::Menu::create(32, 8, 1000, 24, 555))
    , m_controlGuide(strings::get_by_name(strings::names::CONTROL_GUIDES, 3))
    , m_controlGuideX(1220 - sdl::text::get_width(22, m_controlGuide))
    , m_renderTarget(sdl::TextureManager::load(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_TARGET))
{
    SettingsState::load_settings_menu();
    SettingsState::load_extra_strings();
    SettingsState::update_menu_options();
}

void SettingsState::update()
{
    const bool hasFocus     = BaseState::has_focus();
    const bool aPressed     = input::button_pressed(HidNpadButton_A);
    const bool bPressed     = input::button_pressed(HidNpadButton_B);
    const bool minusPressed = input::button_pressed(HidNpadButton_Minus);

    m_settingsMenu->update(hasFocus);
    if (aPressed) { SettingsState::toggle_options(); }
    else if (minusPressed) { SettingsState::create_push_description_message(); }
    else if (bPressed) { BaseState::deactivate(); }
}

void SettingsState::render()
{
    const bool hasFocus = BaseState::has_focus();

    m_renderTarget->clear(colors::TRANSPARENT);
    m_settingsMenu->render(m_renderTarget, hasFocus);
    m_renderTarget->render(sdl::Texture::Null, 201, 91);

    if (hasFocus)
    {
        sdl::text::render(sdl::Texture::Null, m_controlGuideX, 673, 22, sdl::text::NO_WRAP, colors::WHITE, m_controlGuide);
    }
}

void SettingsState::load_settings_menu()
{
    for (int i = 0; const char *option = strings::get_by_name(strings::names::SETTINGS_MENU, i); i++)
    {
        m_settingsMenu->add_option(option);
    }
}

void SettingsState::load_extra_strings()
{
    for (int i = 0; const char *onOff = strings::get_by_name(strings::names::ON_OFF, i); i++) { m_onOff[i] = onOff; }
    for (int i = 0; const char *sortType = strings::get_by_name(strings::names::SORT_TYPES, i); i++)
    {
        m_sortTypes[i] = sortType;
    }
}

void SettingsState::update_menu_options()
{
    for (int i : {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, 20, 21, 22})
    {
        const char *optionFormat = strings::get_by_name(strings::names::SETTINGS_MENU, i);
        const uint8_t value      = config::get_by_key(CONFIG_KEY_ARRAY[i]);
        const char *status       = SettingsState::get_status_text(value);
        const std::string option = stringutil::get_formatted_string(optionFormat, status);
        m_settingsMenu->edit_option(i, option);
    }

    {
        const char *zipCompFormat   = strings::get_by_name(strings::names::SETTINGS_MENU, CYCLE_ZIP);
        const uint8_t zipLevel      = config::get_by_key(CONFIG_KEY_ARRAY[CYCLE_ZIP]);
        const std::string zipOption = stringutil::get_formatted_string(zipCompFormat, zipLevel);
        m_settingsMenu->edit_option(CYCLE_ZIP, zipOption);
    }

    {
        const char *titleSortFormat      = strings::get_by_name(strings::names::SETTINGS_MENU, CYCLE_SORT_TYPE);
        const uint8_t sortType           = config::get_by_key(CONFIG_KEY_ARRAY[CYCLE_SORT_TYPE]);
        const char *typeText             = SettingsState::get_sort_type_text(sortType);
        const std::string sortTypeOption = stringutil::get_formatted_string(titleSortFormat, typeText);
        m_settingsMenu->edit_option(CYCLE_SORT_TYPE, sortTypeOption);
    }

    {
        const char *scalingFormat       = strings::get_by_name(strings::names::SETTINGS_MENU, CYCLE_SCALING);
        const double scaling            = config::get_animation_scaling();
        const std::string scalingOption = stringutil::get_formatted_string(scalingFormat, scaling);
        m_settingsMenu->edit_option(CYCLE_SCALING, scalingOption);
    }
}

void SettingsState::change_working_directory()
{
    const int popTicks           = ui::PopMessageManager::DEFAULT_TICKS;
    const char *inputHeader      = strings::get_by_name(strings::names::KEYBOARD, 2);
    const char *popSuccessFormat = strings::get_by_name(strings::names::SETTINGS_POPS, 1);
    const char *popFailed        = strings::get_by_name(strings::names::SETTINGS_POPS, 2);

    const fslib::Path oldPath{config::get_working_directory()};
    std::array<char, FS_MAX_PATH> pathBuffer = {0};

    const bool input = keyboard::get_input(SwkbdType_Normal, oldPath.full_path(), inputHeader, pathBuffer.data(), FS_MAX_PATH);
    if (!input) { return; }

    const fslib::Path newPath{pathBuffer.data()};
    const bool exists = fslib::directory_exists(newPath);
    if (exists)
    {
        ui::PopMessageManager::push_message(popTicks, popFailed);
        return;
    }

    const bool pathSet     = config::set_working_directory(newPath);
    const bool renameError = pathSet && error::fslib(fslib::rename_directory(oldPath, newPath));
    if (!pathSet || renameError)
    {
        ui::PopMessageManager::push_message(popTicks, popFailed);
        return;
    }

    const std::string popMessage = stringutil::get_formatted_string(popSuccessFormat, newPath.full_path());
    ui::PopMessageManager::push_message(popTicks, popMessage);
}

void SettingsState::create_push_blacklist_edit()
{
    const bool isEmpty = config::blacklist_is_empty();
    if (isEmpty)
    {
        const int popTicks   = ui::PopMessageManager::DEFAULT_TICKS;
        const char *popEmpty = strings::get_by_name(strings::names::SETTINGS_POPS, 0);
        ui::PopMessageManager::push_message(popTicks, popEmpty);
        return;
    }

    BlacklistEditState::create_and_push();
}

void SettingsState::toggle_options()
{
    const int selected = m_settingsMenu->get_selected();
    switch (selected)
    {
        case CHANGE_WORK_DIR: SettingsState::change_working_directory(); break;
        case EDIT_BLACKLIST:  SettingsState::create_push_blacklist_edit(); break;
        case CYCLE_ZIP:       SettingsState::cycle_zip_level(); break;
        case CYCLE_SORT_TYPE: SettingsState::cycle_sort_type(); break;
        case TOGGLE_JKSM:     SettingsState::toggle_jksm_mode(); break;
        case TOGGLE_TRASH:    SettingsState::toggle_trash_folder(); break;
        case CYCLE_SCALING:   SettingsState::cycle_anim_scaling(); break;
        default:              config::toggle_by_key(CONFIG_KEY_ARRAY[selected]); break;
    }
    config::save();
    SettingsState::update_menu_options();
}

void SettingsState::create_push_description_message()
{
    const int selected      = m_settingsMenu->get_selected();
    const char *description = strings::get_by_name(strings::names::SETTINGS_DESCRIPTIONS, selected);

    MessageState::create_and_push_fade(description);
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

void SettingsState::toggle_trash_folder()
{
    const bool trashEnabled = config::get_by_key(config::keys::ENABLE_TRASH_BIN);
    const fslib::Path trashPath{config::get_working_directory() / "_TRASH_"};
    config::toggle_by_key(config::keys::ENABLE_TRASH_BIN);

    if (trashEnabled) { error::fslib(fslib::delete_directory_recursively(trashPath)); }
    else { error::fslib(fslib::create_directory(trashPath)); }
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
    return m_sortTypes[value];
}

#include "appstates/TextTitleSelectState.hpp"
#include "StateManager.hpp"
#include "appstates/BackupMenuState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TitleOptionState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "fs/save_mount.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "logger.hpp"
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

void TextTitleSelectState::update()
{
    m_titleSelectMenu.update(AppState::has_focus());

    // Both title selection states work too differently for this stuff to be shared IMO.
    if (input::button_pressed(HidNpadButton_A))
    {
        // Grab selected.
        int selected = m_titleSelectMenu.get_selected();

        // Grab what we need to continue.
        uint64_t applicationID = m_user->get_application_id_at(selected);
        FsSaveDataInfo *saveInfo = m_user->get_save_info_by_id(applicationID);
        data::TitleInfo *titleInfo = data::get_title_info_by_id(m_user->get_application_id_at(selected));

        // Output path.
        fslib::Path targetPath = config::get_working_directory() / titleInfo->get_path_safe_title();

        if ((fslib::directory_exists(targetPath) || fslib::create_directory(targetPath)) &&
            fslib::open_save_data_with_save_info(fs::DEFAULT_SAVE_MOUNT, *saveInfo))
        {
            // State
            auto backupMenuState =
                std::make_shared<BackupMenuState>(m_user,
                                                  titleInfo,
                                                  static_cast<FsSaveDataType>(saveInfo->save_data_type));

            StateManager::push_state(backupMenuState);
        }
        else
        {
            logger::log(fslib::get_error_string());
        }
    }
    else if (input::button_pressed(HidNpadButton_X))
    {
        int selected = m_titleSelectMenu.get_selected();

        uint64_t applicationID = m_user->get_application_id_at(selected);
        data::TitleInfo *titleInfo = data::get_title_info_by_id(applicationID);

        auto titleOptionState = std::make_shared<TitleOptionState>(m_user, titleInfo, this);

        StateManager::push_state(titleOptionState);
    }
    else if (input::button_pressed(HidNpadButton_Y))
    {
        uint64_t applicationID = m_user->get_application_id_at(m_titleSelectMenu.get_selected());

        config::add_remove_favorite(applicationID);

        // We need to resort all users, not just this one.
        data::UserList list;
        data::get_users(list);
        for (data::User *user : list)
        {
            user->sort_data();
        }

        // Let the main menu state take care of this.
        MainMenuState::refresh_view_states();
    }
    else if (input::button_pressed(HidNpadButton_B))
    {
        AppState::deactivate();
    }
}

void TextTitleSelectState::render()
{
    m_renderTarget->clear(colors::TRANSPARENT);
    m_titleSelectMenu.render(m_renderTarget->get(), AppState::has_focus());
    TitleSelectCommon::render_control_guide();
    m_renderTarget->render(NULL, 201, 91);
}

void TextTitleSelectState::refresh()
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

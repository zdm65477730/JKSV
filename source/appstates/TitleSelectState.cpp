#include "appstates/TitleSelectState.hpp"
#include "JKSV.hpp"
#include "appstates/BackupMenuState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TitleOptionState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include <string_view>

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

TitleSelectState::TitleSelectState(data::User *user)
    : TitleSelectCommon(), m_user(user),
      m_renderTarget(sdl::TextureManager::create_load_texture(SECONDARY_TARGET,
                                                              1080,
                                                              555,
                                                              SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_titleView(m_user) {};

void TitleSelectState::update(void)
{
    if (m_user->get_total_data_entries() <= 0)
    {
        AppState::deactivate();
        return;
    }

    m_titleView.update(AppState::has_focus());

    if (input::button_pressed(HidNpadButton_A))
    {
        // Get data needed to mount save.
        uint64_t applicationID = m_user->get_application_id_at(m_titleView.get_selected());
        FsSaveDataInfo *saveInfo = m_user->get_save_info_by_id(applicationID);
        data::TitleInfo *titleInfo = data::get_title_info_by_id(applicationID);

        // Path to output to.
        fslib::Path targetPath = config::get_working_directory() / titleInfo->get_path_safe_title();

        if ((fslib::directory_exists(targetPath) || fslib::create_directory(targetPath)) &&
            fslib::open_save_data_with_save_info(fs::DEFAULT_SAVE_MOUNT, *saveInfo))
        {
            JKSV::push_state(std::make_shared<BackupMenuState>(m_user,
                                                               titleInfo,
                                                               static_cast<FsSaveDataType>(saveInfo->save_data_type)));
        }
        else
        {
            logger::log(fslib::get_error_string());
        }
    }
    else if (input::button_pressed(HidNpadButton_X))
    {
        uint64_t applicationID = m_user->get_application_id_at(m_titleView.get_selected());
        data::TitleInfo *titleInfo = data::get_title_info_by_id(applicationID);

        JKSV::push_state(std::make_shared<TitleOptionState>(m_user, titleInfo, this));
    }
    else if (input::button_pressed(HidNpadButton_B))
    {
        // This will reset all the tiles so they're 128x128.
        m_titleView.reset();
        AppState::deactivate();
    }
    else if (input::button_pressed(HidNpadButton_Y))
    {
        // Add/remove favorite flag.
        config::add_remove_favorite(m_user->get_application_id_at(m_titleView.get_selected()));

        // Resort the data.
        data::UserList list;
        data::get_users(list);
        for (data::User *user : list)
        {
            user->sort_data();
        }

        MainMenuState::refresh_view_states();
    }
}

void TitleSelectState::render(void)
{
    m_renderTarget->clear(colors::TRANSPARENT);
    m_titleView.render(m_renderTarget->get(), AppState::has_focus());
    TitleSelectCommon::render_control_guide();
    m_renderTarget->render(NULL, 201, 91);
}

void TitleSelectState::refresh(void)
{
    m_titleView.refresh();
}

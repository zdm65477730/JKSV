#include "appstates/TitleSelectState.hpp"
#include "JKSV.hpp"
#include "appstates/BackupMenuState.hpp"
#include "appstates/MainMenuState.hpp"
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
      m_renderTarget(sdl::TextureManager::createLoadTexture(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_titleView(m_user) {};

void TitleSelectState::update(void)
{
    m_titleView.update(AppState::hasFocus());

    if (input::buttonPressed(HidNpadButton_A))
    {
        // Get data needed to mount save.
        uint64_t applicationID = m_user->getApplicationIDAt(m_titleView.getSelected());
        FsSaveDataInfo *saveInfo = m_user->getSaveInfoByID(applicationID);
        data::TitleInfo *titleInfo = data::getTitleInfoByID(applicationID);

        // Path to output to.
        fslib::Path targetPath = config::getWorkingDirectory() / titleInfo->getPathSafeTitle();

        if ((fslib::directoryExists(targetPath) || fslib::createDirectory(targetPath)) &&
            fslib::openSaveFileSystemWithSaveDataInfo(fs::DEFAULT_SAVE_MOUNT, *saveInfo))
        {
            JKSV::pushState(std::make_shared<BackupMenuState>(m_user, titleInfo, static_cast<FsSaveDataType>(saveInfo->save_data_type)));
        }
        else
        {
            logger::log("%s", fslib::getErrorString());
        }
    }
    else if (input::buttonPressed(HidNpadButton_B))
    {
        // This will reset all the tiles so they're 128x128.
        m_titleView.reset();
        AppState::deactivate();
    }
    else if (input::buttonPressed(HidNpadButton_Y))
    {
        config::addRemoveFavorite(m_user->getApplicationIDAt(m_titleView.getSelected()));
        // MainMenuState has all the Users and views, so have it refresh.
        MainMenuState::refreshViewStates();
    }
}

void TitleSelectState::render(void)
{
    m_renderTarget->clear(colors::TRANSPARENT);
    m_titleView.render(m_renderTarget->get(), AppState::hasFocus());
    TitleSelectCommon::renderControlGuide();
    m_renderTarget->render(NULL, 201, 91);
}

void TitleSelectState::refresh(void)
{
    m_titleView.refresh();
}

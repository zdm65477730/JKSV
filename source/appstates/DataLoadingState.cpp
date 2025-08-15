#include "appstates/DataLoadingState.hpp"

#include "colors.hpp"
#include "logger.hpp"

namespace
{
    constexpr int SCREEN_CENTER = 640;
}

DataLoadingState::~DataLoadingState()
{
    // This is to catch stragglers.
    m_context.process_icon_queue();
    if (m_destructFunction) { m_destructFunction(); }
}

void DataLoadingState::update()
{
    static constexpr int SCREEN_CENTER = 640;

    m_context.process_icon_queue();

    BaseTask::update();
    const std::string status = m_task->get_status();
    const int statusWidth    = sdl::text::get_width(22, status);
    m_statusX                = SCREEN_CENTER - (statusWidth / 2);
}

void DataLoadingState::render()
{
    static constexpr int ICON_X_COORD = SCREEN_CENTER - 128;
    static constexpr int ICON_Y_COORD = 226;
    const std::string status          = m_task->get_status();

    sdl::render_rect_fill(sdl::Texture::Null, 0, 0, 1280, 720, colors::CLEAR_COLOR);
    sm_jksvIcon->render(sdl::Texture::Null, ICON_X_COORD, ICON_Y_COORD);
    sdl::text::render(sdl::Texture::Null, m_statusX, 673, 22, sdl::text::NO_WRAP, colors::WHITE, status);
    BaseTask::render_loading_glyph();
}

void DataLoadingState::initialize_static_members()
{
    if (sm_jksvIcon) { return; }

    sm_jksvIcon = sdl::TextureManager::create_load_texture("LoadingIcon", "romfs:/Textures/LoadingIcon.png");
}

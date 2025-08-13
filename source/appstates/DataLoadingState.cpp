#include "appstates/DataLoadingState.hpp"

#include "appstates/FadeState.hpp"
#include "colors.hpp"

namespace
{
    constexpr int SCREEN_CENTER = 640;
}

DataLoadingState::~DataLoadingState() { DataLoadingState::execute_destructs(); }

void DataLoadingState::update()
{
    static constexpr int SCREEN_CENTER = 640;

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

void DataLoadingState::execute_destructs()
{
    for (auto &function : m_destructFunctions) { function(); }
}

void DataLoadingState::add_destruct_function(DataLoadingState::DestructFunction function)
{
    m_destructFunctions.push_back(function);
}

void DataLoadingState::initialize_static_members()
{
    if (sm_jksvIcon) { return; }

    sm_jksvIcon = sdl::TextureManager::create_load_texture("LoadingIcon", "romfs:/Textures/LoadingIcon.png");
}

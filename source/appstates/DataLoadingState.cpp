#include "appstates/DataLoadingState.hpp"

#include "graphics/colors.hpp"
#include "logging/logger.hpp"

namespace
{
    constexpr int SCREEN_CENTER = 640;
}

DataLoadingState::DataLoadingState(data::DataContext &context,
                                   DestructFunction destructFunction,
                                   sys::threadpool::JobFunction function,
                                   sys::Task::TaskData taskData)
    : BaseTask()
    , m_context(context)
    , m_destructFunction(destructFunction)
{
    DataLoadingState::initialize_static_members();
    m_task = std::make_unique<sys::Task>(function, taskData);
}

void DataLoadingState::update()
{
    BaseTask::update_loading_glyph();
    if (!m_task->is_running()) { DataLoadingState::deactivate_state(); }
    m_context.process_icon_queue();
}

void DataLoadingState::render()
{
    static constexpr int ICON_X_COORD = SCREEN_CENTER - 128;
    static constexpr int ICON_Y_COORD = 226;
    const std::string status          = m_task->get_status();

    const int statusWidth = sdl::text::get_width(BaseTask::FONT_SIZE, status);
    m_statusX             = SCREEN_CENTER - (statusWidth / 2);

    sdl::render_rect_fill(sdl::Texture::Null, 0, 0, 1280, 720, colors::CLEAR_COLOR);
    sm_jksvIcon->render(sdl::Texture::Null, ICON_X_COORD, ICON_Y_COORD);
    sdl::text::render(sdl::Texture::Null, m_statusX, 673, BaseTask::FONT_SIZE, sdl::text::NO_WRAP, colors::WHITE, status);
    BaseTask::render_loading_glyph();
}

void DataLoadingState::initialize_static_members()
{
    if (sm_jksvIcon) { return; }

    sm_jksvIcon = sdl::TextureManager::load("LoadingIcon", "romfs:/Textures/LoadingIcon.png");
}

void DataLoadingState::deactivate_state()
{
    // This is to catch any stragglers.
    m_context.process_icon_queue();
    if (m_destructFunction) { m_destructFunction(); }
    BaseState::deactivate();
}

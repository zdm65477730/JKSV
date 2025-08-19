#include "appstates/TaskState.hpp"

#include "appstates/FadeState.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"
#include "ui/PopMessageManager.hpp"

void TaskState::update()
{
    BaseTask::update_loading_glyph();
    if (!m_task->is_running()) { TaskState::deactivate_state(); }
}

void TaskState::render()
{
    const std::string status = m_task->get_status();
    const int statusX        = 640 - (sdl::text::get_width(BaseTask::FONT_SIZE, status.c_str()) / 2);

    sdl::render_rect_fill(sdl::Texture::Null, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
    sdl::text::render(sdl::Texture::Null, statusX, 351, BaseTask::FONT_SIZE, sdl::text::NO_WRAP, colors::WHITE, status);

    BaseTask::render_loading_glyph();
}

void TaskState::deactivate_state()
{
    FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_END, colors::ALPHA_FADE_BEGIN, nullptr);
    BaseState::deactivate();
}

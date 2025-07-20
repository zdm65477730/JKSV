#include "appstates/TaskState.hpp"

#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "ui/PopMessageManager.hpp"

void TaskState::update() { BaseTask::update(); }

void TaskState::render()
{
    const std::string status = m_task->get_status();
    const int statusX        = 640 - (sdl::text::get_width(24, status.c_str()) / 2);

    sdl::render_rect_fill(NULL, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
    sdl::text::render(NULL, statusX, 351, 24, sdl::text::NO_TEXT_WRAP, colors::WHITE, status.c_str());

    BaseTask::render_loading_glyph();
}

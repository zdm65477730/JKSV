#include "appstates/ProgressState.hpp"

#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"
#include "ui/render_functions.hpp"

#include <cmath>

void ProgressState::update()
{
    static constexpr double SIZE_BAR_WIDTH = 656.0f;

    sys::ProgressTask *task = static_cast<sys::ProgressTask *>(m_task.get());
    const double current    = task->get_progress();

    // Base routine.
    BaseTask::update();

    m_progressBarWidth = std::ceil(SIZE_BAR_WIDTH * current);
    m_progress         = std::ceil(current * 100);
    m_percentageString = stringutil::get_formatted_string("%u", m_progress);
    m_percentageX      = 640 - (sdl::text::get_width(18, m_percentageString.c_str()));
}

void ProgressState::render()
{
    const std::string status = m_task->get_status();
    const char *percentage   = m_percentageString.c_str();

    sdl::render_rect_fill(NULL, 0, 0, 1280, 720, colors::DIM_BACKGROUND);

    ui::render_dialog_box(NULL, 280, 262, 720, 256);
    sdl::text::render(NULL, 312, 288, 18, 648, colors::WHITE, status.c_str());
    sdl::render_rect_fill(NULL, 312, 462, 656, 32, colors::BLACK);
    sdl::render_rect_fill(NULL, 312, 462, m_progressBarWidth, 32, colors::GREEN);
    sdl::text::render(NULL, m_percentageX, 468, 18, sdl::text::NO_TEXT_WRAP, colors::WHITE, "%s%%", percentage);

    BaseTask::render_loading_glyph();
}

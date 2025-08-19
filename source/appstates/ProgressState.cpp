#include "appstates/ProgressState.hpp"

#include "appstates/FadeState.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"

#include <cmath>

namespace
{
    constexpr int COORD_BAR_X = 312;
    constexpr int COORD_BAR_Y = 462;

    constexpr int COORD_TEXT_Y = 467;

    constexpr int COORD_DISPLAY_CENTER = 640;
    constexpr double SIZE_BAR_WIDTH    = 656.0f;
}

void ProgressState::update()
{
    sys::ProgressTask *task = static_cast<sys::ProgressTask *>(m_task.get());
    const double current    = task->get_progress();

    BaseTask::update_loading_glyph();
    if (!m_task->is_running()) { ProgressState::deactivate_state(); }

    m_progressBarWidth        = std::round(SIZE_BAR_WIDTH * current);
    m_progress                = std::round(current * 100);
    m_percentageString        = stringutil::get_formatted_string("%u%%", m_progress);
    const int percentageWidth = sdl::text::get_width(BaseTask::FONT_SIZE, m_percentageString);
    m_percentageX             = COORD_DISPLAY_CENTER - (percentageWidth / 2);
}

void ProgressState::render()
{
    const bool hasFocus      = BaseState::has_focus();
    const int barWidth       = static_cast<int>(SIZE_BAR_WIDTH);
    const std::string status = m_task->get_status();

    sdl::render_rect_fill(sdl::Texture::Null, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
    sm_dialog->render(sdl::Texture::Null, hasFocus);
    sdl::text::render(sdl::Texture::Null, 312, 288, BaseTask::FONT_SIZE, 656, colors::WHITE, status);

    sdl::render_rect_fill(sdl::Texture::Null, COORD_BAR_X, COORD_BAR_Y, barWidth, 32, colors::BLACK);
    sdl::render_rect_fill(sdl::Texture::Null, COORD_BAR_X, COORD_BAR_Y, m_progressBarWidth, 32, colors::GREEN);
    sdl::text::render(sdl::Texture::Null,
                      m_percentageX,
                      COORD_TEXT_Y,
                      BaseTask::FONT_SIZE,
                      sdl::text::NO_WRAP,
                      colors::WHITE,
                      m_percentageString);

    BaseTask::render_loading_glyph();
}

void ProgressState::initialize_static_members()
{
    if (sm_dialog) { return; }

    sm_dialog = ui::DialogBox::create(280, 262, 720, 256);
}

void ProgressState::deactivate_state()
{
    FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_END, colors::ALPHA_FADE_BEGIN, nullptr);
    BaseState::deactivate();
}

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
    constexpr int COORD_BAR_X = 296;
    constexpr int COORD_BAR_Y = 437;

    constexpr int COORD_TEXT_Y = 442;

    constexpr int COORD_DISPLAY_CENTER = 640;
    constexpr double SIZE_BAR_WIDTH    = 688.0f;
}

ProgressState::ProgressState(sys::threadpool::JobFunction function, sys::Task::TaskData taskData)
    : m_transition(0, 0, 32, 32, 0, 0, 720, 256, ui::Transition::DEFAULT_THRESHOLD)
{
    initialize_static_members();
    m_task = std::make_unique<sys::ProgressTask>(function, taskData);
}

void ProgressState::update()
{
    BaseTask::update_loading_glyph();
    BaseTask::pop_on_plus();
    m_transition.update();
    sm_dialog->set_from_transition(m_transition, true);
    if (!m_transition.in_place()) { return; }

    sys::ProgressTask *task = static_cast<sys::ProgressTask *>(m_task.get());
    const double current    = task->get_progress();

    const bool isRunning = m_task->is_running();
    if (!isRunning && !m_close) { ProgressState::close_dialog(); }
    else if (m_close && m_transition.in_place()) { ProgressState::deactivate_state(); }

    m_progressBarWidth        = std::round(SIZE_BAR_WIDTH * current);
    m_progress                = std::round(current * 100);
    m_percentageString        = stringutil::get_formatted_string("%u%%", m_progress);
    const int percentageWidth = sdl::text::get_width(BaseTask::FONT_SIZE, m_percentageString);
    m_percentageX             = COORD_DISPLAY_CENTER - (percentageWidth / 2);
}

void ProgressState::render()
{
    static constexpr int RIGHT_EDGE_X = (COORD_BAR_X + SIZE_BAR_WIDTH) - 16;
    const bool hasFocus               = BaseState::has_focus();

    sdl::render_rect_fill(sdl::Texture::Null, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
    sm_dialog->render(sdl::Texture::Null, hasFocus);
    BaseTask::render_loading_glyph();
    if (!m_transition.in_place()) { return; }

    const int barWidth       = static_cast<int>(SIZE_BAR_WIDTH);
    const std::string status = m_task->get_status();
    sdl::text::render(sdl::Texture::Null, 312, 255, BaseTask::FONT_SIZE, 656, colors::WHITE, status);

    sdl::render_line(sdl::Texture::Null, 280, 421, 999, 421, colors::DIV_COLOR);
    sdl::render_rect_fill(sdl::Texture::Null, COORD_BAR_X, COORD_BAR_Y, barWidth, 32, colors::BLACK);
    sdl::render_rect_fill(sdl::Texture::Null, COORD_BAR_X, COORD_BAR_Y, m_progressBarWidth, 32, colors::BAR_GREEN);

    sm_barEdges->render_part(sdl::Texture::Null, COORD_BAR_X, COORD_BAR_Y, 0, 0, 16, 32);
    sm_barEdges->render_part(sdl::Texture::Null, RIGHT_EDGE_X, COORD_BAR_Y, 16, 0, 16, 32);

    sdl::text::render(sdl::Texture::Null,
                      m_percentageX,
                      COORD_TEXT_Y,
                      BaseTask::FONT_SIZE,
                      sdl::text::NO_WRAP,
                      colors::WHITE,
                      m_percentageString);
}

void ProgressState::initialize_static_members()
{
    static constexpr std::string_view BAR_EDGE_NAME = "BarEdges";

    if (sm_dialog && sm_barEdges) { return; }

    sm_dialog   = ui::DialogBox::create(0, 0, 0, 0);
    sm_barEdges = sdl::TextureManager::load(BAR_EDGE_NAME, "romfs:/Textures/BarEdges.png");

    sm_dialog->set_from_transition(m_transition, true);
}

void ProgressState::close_dialog()
{
    m_transition.set_target_width(32);
    m_transition.set_target_height(32);
    m_close = true;
}

void ProgressState::deactivate_state()
{
    FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_END, colors::ALPHA_FADE_BEGIN, nullptr);
    BaseState::deactivate();
}

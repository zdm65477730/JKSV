#include "appstates/ProgressState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"
#include "ui/render_functions.hpp"
#include <cmath>

void ProgressState::update(void)
{
    // Base routine.
    BaseTask::update();

    if (m_task.is_running() && input::button_pressed(HidNpadButton_Plus))
    {
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_BACKUP_MENU, 0));
    }
    else if (!m_task.is_running())
    {
        AppState::deactivate();
    }

    m_progressBarWidth = std::ceil(656.0f * m_task.get_current());
    m_progress = std::ceil(m_task.get_current() * 100);
    m_percentageString = stringutil::get_formatted_string("%u", m_progress);
    m_percentageX = 640 - (sdl::text::get_width(18, m_percentageString.c_str()));
}

void ProgressState::render(void)
{
    // This will dim the background.
    sdl::render_rect_fill(NULL, 0, 0, 1280, 720, colors::DIM_BACKGROUND);

    // Render the dialog and little loading bar thingy.
    ui::render_dialog_box(NULL, 280, 262, 720, 256);
    sdl::text::render(NULL, 312, 288, 18, 648, colors::WHITE, m_task.get_status().c_str());
    sdl::render_rect_fill(NULL, 312, 462, 656, 32, colors::BLACK);
    sdl::render_rect_fill(NULL, 312, 462, m_progressBarWidth, 32, colors::GREEN);
    sdl::text::render(NULL,
                      m_percentageX,
                      468,
                      18,
                      sdl::text::NO_TEXT_WRAP,
                      colors::WHITE,
                      "%s%%",
                      m_percentageString.c_str());

    // Glyph in the corner.
    BaseTask::render_loading_glyph();
}

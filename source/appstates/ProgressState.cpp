#include "appstates/ProgressState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "stringUtil.hpp"
#include "strings.hpp"
#include "ui/renderFunctions.hpp"
#include <cmath>

void ProgressState::update(void)
{
    if (!m_task.isRunning())
    {
        AppState::deactivate();
    }

    m_progressBarWidth = std::ceil(656.0f * m_task.getCurrent());
    m_progress = std::ceil(m_task.getCurrent() * 100);
    m_percentageString = stringutil::getFormattedString("%u", m_progress);
    m_percentageX = 640 - (sdl::text::getWidth(18, m_percentageString.c_str()));
}

void ProgressState::render(void)
{
    // This will dim the background.
    sdl::renderRectFill(NULL, 0, 0, 1280, 720, colors::DIM_BACKGROUND);

    // Render the dialog and little loading bar thingy.
    ui::renderDialogBox(NULL, 280, 262, 720, 256);
    sdl::text::render(NULL, 312, 288, 18, 648, colors::WHITE, m_task.getStatus().c_str());
    sdl::renderRectFill(NULL, 312, 462, 656, 32, colors::BLACK);
    sdl::renderRectFill(NULL, 312, 462, m_progressBarWidth, 32, colors::GREEN);
    sdl::text::render(NULL, m_percentageX, 468, 18, sdl::text::NO_TEXT_WRAP, colors::WHITE, "%s%%", m_percentageString.c_str());
}

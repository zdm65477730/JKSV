#include "AppStates/ProgressState.hpp"
#include "Colors.hpp"
#include "SDL.hpp"
#include "StringUtil.hpp"
#include "Strings.hpp"
#include "UI/RenderFunctions.hpp"
#include <cmath>

#include "Input.hpp"

void ProgressState::Update(void)
{
    if (!m_Task.IsRunning())
    {
        AppState::Deactivate();
    }

    m_ProgressBarWidth = std::ceil(656.0f * m_Task.GetCurrentProgress());
    m_Progress = std::ceil(m_Task.GetCurrentProgress() * 100);
    m_PercentageString = StringUtil::GetFormattedString("%u", m_Progress);
    m_PerentageX = 640 - (SDL::Text::GetWidth(18, m_PercentageString.c_str()));
}

void ProgressState::Render(void)
{
    // This will dim the background.
    SDL::RenderRectFill(NULL, 0, 0, 1280, 720, Colors::BackgroundDim);

    // Render the dialog and little loading bar thingy.
    UI::RenderDialogBox(NULL, 280, 262, 720, 256);
    SDL::Text::Render(NULL, 312, 288, 18, 648, Colors::White, m_Task.GetStatus().c_str());
    SDL::RenderRectFill(NULL, 312, 462, 656, 32, Colors::Black);
    SDL::RenderRectFill(NULL, 312, 462, m_ProgressBarWidth, 32, Colors::Green);
    SDL::Text::Render(NULL, m_PerentageX, 468, 18, SDL::Text::NO_TEXT_WRAP, Colors::White, "%s%%", m_PercentageString.c_str());
}

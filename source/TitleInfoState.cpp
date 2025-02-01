#include "appstates/TitleInfoState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"

TitleInfoState::TitleInfoState(data::User *user, data::TitleInfo *titleInfo) : m_user(user), m_titleInfo(titleInfo), m_titleScrollTimer(3000)
{
    if (!sm_initialized)
    {
        // This is the slide panel everything is rendered to.
        sm_slidePanel = std::make_unique<ui::SlideOutPanel>(480, ui::SlideOutPanel::Side::Right);
        sm_initialized = true;
    }

    // Check if the title is too large to fit within the target.
    m_titleWidth = sdl::text::getWidth(32, m_titleInfo->getTitle());
    if (m_titleWidth > 480)
    {
        // Just set this to 8 and we'll scroll the title.
        m_titleX = 8;
        m_titleScrolling = true;
    }
    else
    {
        // Just center it.
        m_titleX = 240 - (m_titleWidth / 2);
    }
}

void TitleInfoState::update(void)
{
    // Update slide panel.
    sm_slidePanel->update(AppState::hasFocus());

    if (input::buttonPressed(HidNpadButton_B))
    {
        sm_slidePanel->close();
    }
    else if (sm_slidePanel->isClosed())
    {
        sm_slidePanel->reset();
    }
    else if (m_titleScrolling && m_titleScrollTimer.isTriggered())
    {
        m_titleX -= 2;
        m_titleScrollTriggered = true;
    }
    else if (m_titleScrollTriggered && m_titleX > 8 - m_titleWidth)
    {
        m_titleX -= 2;
    }
    else if (m_titleScrollTriggered && m_titleX <= 8 - m_titleWidth)
    {
        m_titleX = 8;
        m_titleScrollTriggered = false;
        m_titleScrollTimer.restart();
    }
}

void TitleInfoState::render(void)
{
    // Grab the panel's target and clear it. To do: This how I originally intended to.
    sm_slidePanel->clearTarget();
    // SDL_Texture *panelTarget = sm_slidePanel->get();

    // If the title doesn't need to be scrolled, just render it.


    sdl::renderLine(sm_slidePanel->get(), 10, 42, 460, 42, colors::WHITE);
}

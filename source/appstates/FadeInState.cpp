#include "appstates/FadeInState.hpp"

#include "StateManager.hpp"
#include "sdl.hpp"

FadeInState::FadeInState(std::shared_ptr<BaseState> nextState)
    : m_nextState(nextState)
{
    m_fadeTimer.start(1);
}

void FadeInState::update()
{
    if (m_alpha == 0x00)
    {
        StateManager::push_state(m_nextState);
        BaseState::deactivate();
    }
    else if (m_fadeTimer.is_triggered()) { m_alpha -= 15; }
}

void FadeInState::render()
{
    m_nextState->render();
    const uint32_t rawColor = 0x000000 | m_alpha;
    const sdl::Color fadeColor{rawColor};
    sdl::render_rect_fill(nullptr, 0, 0, 1280, 720, fadeColor);
}

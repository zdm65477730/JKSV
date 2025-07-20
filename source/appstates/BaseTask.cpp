#include "appstates/BaseTask.hpp"

#include "colors.hpp"

namespace
{
    /// @brief This is the time in milliseconds between changing glyphs.
    constexpr uint64_t TICKS_GLYPH_TRIGGER = 50;
} // namespace

BaseTask::BaseTask()
    : BaseState(false)
{
    m_frameTimer.start(TICKS_GLYPH_TRIGGER);
}

void BaseTask::update()
{
    if (!m_task->is_running())
    {
        BaseState::deactivate();
        return;
    }

    // Just bail if the timer wasn't triggered yet.
    if (!m_frameTimer.is_triggered()) { return; }
    if (++m_currentFrame >= 8) { m_currentFrame = 0; }
    m_colorMod.update();
}

void BaseTask::render_loading_glyph()
{
    const char *currentFrame = sm_glyphArray.at(m_currentFrame).data();
    sdl::text::render(NULL, 56, 673, 32, sdl::text::NO_TEXT_WRAP, m_colorMod, currentFrame);
}

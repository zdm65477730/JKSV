#include "appstates/BaseTask.hpp"
#include "colors.hpp"

namespace
{
    /// @brief This is the time in milliseconds between changing glyphs.
    constexpr uint64_t TICKS_GLYPH_TRIGGER = 50;
} // namespace

BaseTask::BaseTask() : BaseState(false)
{
    m_frameTimer.start(TICKS_GLYPH_TRIGGER);
}

void BaseTask::update()
{
    // Just bail if the timer wasn't triggered yet.
    if (!m_frameTimer.is_triggered())
    {
        return;
    }

    // Reset to 0 here.
    if (++m_currentFrame >= 8)
    {
        m_currentFrame = 0;
    }

    // Update the color pulse.
    m_colorMod.update();
}

void BaseTask::render_loading_glyph()
{
    // This assumes it's being called after the background was dimmed.
    sdl::text::render(NULL, 56, 673, 32, sdl::text::NO_TEXT_WRAP, m_colorMod, sm_glyphArray.at(m_currentFrame).data());
}

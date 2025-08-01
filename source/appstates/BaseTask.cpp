#include "appstates/BaseTask.hpp"

#include "colors.hpp"
#include "input.hpp"
#include "strings.hpp"
#include "ui/PopMessageManager.hpp"

namespace
{
    /// @brief This is the time in milliseconds between changing glyphs.
    constexpr uint64_t TICKS_GLYPH_TRIGGER = 50;
} // namespace

BaseTask::BaseTask()
    : BaseState{false}
    , m_popUnableExit{strings::get_by_name(strings::names::GENERAL_POPS, 0)}
{
    m_frameTimer.start(TICKS_GLYPH_TRIGGER);
}

void BaseTask::update()
{
    const bool plusPressed = input::button_pressed(HidNpadButton_Plus);

    if (!m_task->is_running())
    {
        BaseState::deactivate();
        return;
    }
    else if (plusPressed) { ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS, m_popUnableExit); }

    m_colorMod.update();

    // Just bail if the timer wasn't triggered yet.
    if (!m_frameTimer.is_triggered()) { return; }
    if (++m_currentFrame >= 8) { m_currentFrame = 0; }
}

void BaseTask::render_loading_glyph()
{
    const char *currentFrame = sm_glyphArray[m_currentFrame].data();

    sdl::text::render(NULL, 56, 673, 32, sdl::text::NO_TEXT_WRAP, m_colorMod, currentFrame);
}

#include "appstates/TaskState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "ui/PopMessageManager.hpp"

void TaskState::update()
{
    // Run the base update routine.
    BaseTask::update();

    if (m_task.is_running() && input::button_pressed(HidNpadButton_Plus))
    {
        // Throw the message.
        ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                            strings::get_by_name(strings::names::POP_MESSAGES_GENERAL, 0));
    }
    if (!m_task.is_running())
    {
        AppState::deactivate();
    }
}

void TaskState::render()
{
    // Grab task string.
    std::string status = m_task.get_status();
    // Center so it looks perty
    int statusX = 640 - (sdl::text::get_width(24, status.c_str()) / 2);
    // Dim the background states.
    sdl::render_rect_fill(NULL, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
    // Render the status.
    sdl::text::render(NULL, statusX, 351, 24, sdl::text::NO_TEXT_WRAP, colors::WHITE, status.c_str());
    // Render the loading glyph
    BaseTask::render_loading_glyph();
}

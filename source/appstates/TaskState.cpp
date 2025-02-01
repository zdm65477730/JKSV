#include "appstates/TaskState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "ui/PopMessageManager.hpp"

void TaskState::update(void)
{
    if (m_task.isRunning() && input::buttonPressed(HidNpadButton_Plus))
    {
        // Throw the message.
        ui::PopMessageManager::pushMessage(ui::PopMessageManager::DEFAULT_MESSAGE_TICKS,
                                           strings::getByName(strings::names::POP_MESSAGES_GENERAL, 0));
    }
    if (!m_task.isRunning())
    {
        AppState::deactivate();
    }
}

void TaskState::render(void)
{
    // Grab task string.
    std::string status = m_task.getStatus();
    // Center so it looks perty
    int statusX = 640 - (sdl::text::getWidth(24, status.c_str()) / 2);
    // Dim the background states.
    sdl::renderRectFill(NULL, 0, 0, 1280, 720, colors::DIM_BACKGROUND);
    // Render the status.
    sdl::text::render(NULL, statusX, 351, 24, sdl::text::NO_TEXT_WRAP, colors::WHITE, status.c_str());
}

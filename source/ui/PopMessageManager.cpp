#include "ui/PopMessageManager.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "sdl.hpp"
#include "ui/render_functions.hpp"
#include <cstdarg>

namespace
{
    // Size of the buffer for va strings.
    constexpr int VA_BUFFER_SIZE = 0x200;

} // namespace

void ui::PopMessageManager::update(void)
{
    // Grab instance.
    PopMessageManager &manager = PopMessageManager::get_instance();

    // Bail if the queue is empty.
    if (!manager.m_messageQueue.empty())
    {
        // Loop through the queue and process it so we don't wind up with black characters.
        for (auto &[displayTicks, currentMessage] : manager.m_messageQueue)
        {
            // New message.
            manager.m_messages.push_back({.m_y = 720,
                                          .m_targetY = 720,
                                          .m_width = sdl::text::get_width(32, currentMessage.c_str()) + 32,
                                          .m_message = currentMessage,
                                          .m_timer = sys::Timer(displayTicks)});
        }
        // Clear the queue.
        manager.m_messageQueue.clear();
    }

    // Update all the messages.
    // This is the first Y position a message should be displayed at.;
    double currentY = 594.0f;
    double animationScaling = config::get_animation_scaling();
    for (size_t i = 0; i < manager.m_messages.size(); i++)
    {
        // Save myself a shit load of typing.
        ui::PopMessage &currentMessage = manager.m_messages.at(i);

        // Purge it and continue if needed.
        if (currentMessage.m_timer.is_triggered())
        {
            manager.m_messages.erase(manager.m_messages.begin() + i);
            continue;
        }

        // Make sure Y coordinate is correct.
        if (currentMessage.m_targetY != currentY)
        {
            currentMessage.m_targetY = currentY;
        }

        if (currentMessage.m_y != currentMessage.m_targetY)
        {
            currentMessage.m_y += (currentMessage.m_targetY - currentMessage.m_y) / animationScaling;
        }
        currentY -= 56;
    }
}

void ui::PopMessageManager::render(void)
{
    // Get instance.
    PopMessageManager &manager = PopMessageManager::get_instance();

    // Loop and render.
    for (auto &popMessage : manager.m_messages)
    {
        // Render a dialog box around it.
        ui::render_dialog_box(NULL, 20, popMessage.m_y - 6, popMessage.m_width, 52);
        // Render the actual text.
        sdl::text::render(NULL,
                          36,
                          popMessage.m_y,
                          32,
                          sdl::text::NO_TEXT_WRAP,
                          colors::WHITE,
                          popMessage.m_message.c_str());
    }
}

void ui::PopMessageManager::push_message(int displayTicks, const char *format, ...)
{
    // VA args.
    char vaBuffer[VA_BUFFER_SIZE] = {0};

    std::va_list vaList;
    va_start(vaList, format);
    vsnprintf(vaBuffer, VA_BUFFER_SIZE, format, vaList);
    va_end(vaList);

    // Get instance.
    PopMessageManager &manager = PopMessageManager::get_instance();

    // Make sure we're not pushing two of the same message.
    if (!manager.m_messages.empty() && manager.m_messages.back().m_message.compare(vaBuffer) == 0)
    {
        // Bail and don't push it to the queue because it matches.
        return;
    }
    // Push it to the queue.
    std::scoped_lock<std::mutex> messageLock(manager.m_messageMutex);
    manager.m_messageQueue.push_back(std::make_pair(displayTicks, vaBuffer));
}

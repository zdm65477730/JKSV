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

void ui::PopMessageManager::update()
{
    // Grab instance.
    PopMessageManager &manager = PopMessageManager::get_instance();

    auto &messageQueue = manager.m_messageQueue;
    auto &messages     = manager.m_messages;

    // Bail if the queue is empty.
    if (!messageQueue.empty())
    {
        // Loop through the queue and process it so we don't wind up with black characters.
        for (auto &[displayTicks, currentMessage] : messageQueue)
        {
            const size_t messageWidth       = sdl::text::get_width(32, currentMessage.c_str()) + 32;
            const ui::PopMessage newMessage = {.y       = 720,
                                               .targetY = 720,
                                               .width   = messageWidth,
                                               .message = std::move(currentMessage),
                                               .timer   = sys::Timer{displayTicks}};

            messages.push_back(std::move(newMessage));
        }
        messageQueue.clear();
    }

    // Update all the messages.
    // This is the first Y position a message should be displayed at.;
    double currentY               = 594.0f;
    const double animationScaling = config::get_animation_scaling();
    const int messageCount        = messages.size();
    for (int i = 0; i < messageCount; i++)
    {
        ui::PopMessage &message = messages.at(i);
        if (message.timer.is_triggered())
        {
            messages.erase(messages.begin() + i);
            continue;
        }

        if (message.targetY != currentY) { message.targetY = currentY; }
        if (message.y != message.targetY) { message.y += (message.targetY - message.y) / animationScaling; }
        currentY -= 56;
    }
}

void ui::PopMessageManager::render()
{
    PopMessageManager &manager = PopMessageManager::get_instance();
    auto &messages             = manager.m_messages;

    for (const auto &message : messages)
    {
        ui::render_dialog_box(nullptr, 20, message.y - 6, message.width, 52);
        sdl::text::render(nullptr, 36, message.y, 32, sdl::text::NO_TEXT_WRAP, colors::WHITE, message.message.c_str());
    }
}

void ui::PopMessageManager::push_message(int displayTicks, std::string_view message)
{
    PopMessageManager &manager = PopMessageManager::get_instance();
    std::mutex &messageMutex   = manager.m_messageMutex;
    auto &messageQueue         = manager.m_messageQueue;
    auto &messages             = manager.m_messages;

    const bool empty       = messages.empty();
    const bool backMatches = !empty && messages.back().message == message;
    if (backMatches) { return; }

    auto queuePair = std::make_pair(displayTicks, std::string{message});
    std::scoped_lock<std::mutex> messageLock(messageMutex);
    messageQueue.push_back(std::move(queuePair));
}

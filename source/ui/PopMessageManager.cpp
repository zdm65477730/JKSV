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
    static constexpr double COORD_INIT_Y = 594.0f;

    // Grab instance.
    PopMessageManager &manager = PopMessageManager::get_instance();

    auto &messageQueue       = manager.m_messageQueue;
    auto &messages           = manager.m_messages;
    std::mutex &messageMutex = manager.m_messageMutex;
    std::mutex &queueMutex   = manager.m_queueMutex;

    {
        std::lock_guard<std::mutex> queueGuard{queueMutex};
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
                                                   .timer   = sys::Timer{static_cast<uint64_t>(displayTicks)}};

                messages.push_back(std::move(newMessage));
            }
            messageQueue.clear();
        }
    }

    // Update all the messages.
    // This is the first Y position a message should be displayed at.;
    double currentY               = COORD_INIT_Y;
    const double animationScaling = config::get_animation_scaling();
    std::lock_guard<std::mutex> messageGuard{messageMutex};
    for (auto message = messages.begin(); message != messages.end();)
    {
        if (message->timer.is_triggered())
        {
            message = messages.erase(message);
            continue;
        }

        if (message->targetY != currentY) { message->targetY = currentY; }
        else if (message->y != message->targetY) { message->y += (message->targetY - message->y) / animationScaling; }
        currentY -= 56;
        ++message;
    }
}

void ui::PopMessageManager::render()
{
    PopMessageManager &manager = PopMessageManager::get_instance();
    auto &messages             = manager.m_messages;
    std::mutex &messageMutex   = manager.m_messageMutex;

    std::lock_guard<std::mutex> messageGuard{messageMutex};
    for (const auto &message : messages)
    {
        ui::render_dialog_box(nullptr, 20, message.y - 6, message.width, 52);
        sdl::text::render(nullptr, 36, message.y, 32, sdl::text::NO_TEXT_WRAP, colors::WHITE, message.message.c_str());
    }
}

void ui::PopMessageManager::push_message(int displayTicks, std::string_view message)
{
    PopMessageManager &manager = PopMessageManager::get_instance();
    std::mutex &queueMutex     = manager.m_queueMutex;
    auto &messageQueue         = manager.m_messageQueue;

    std::lock_guard<std::mutex> queueGuard(queueMutex);
    auto queuePair = std::make_pair(displayTicks, std::string{message});
    messageQueue.push_back(std::move(queuePair));
}

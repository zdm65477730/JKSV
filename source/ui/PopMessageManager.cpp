#include "ui/PopMessageManager.hpp"

#include "config/config.hpp"
#include "graphics/colors.hpp"
#include "logging/logger.hpp"
#include "sdl.hpp"
#include "ui/PopMessage.hpp"

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
            for (auto &[displayTicks, currentMessage] : messageQueue) { messages.emplace_back(displayTicks, currentMessage); }
            messageQueue.clear();
        }
    }

    // Update all the messages.
    // This is the first Y position a message should be displayed at.;
    double currentY = COORD_INIT_Y;
    std::lock_guard<std::mutex> messageGuard{messageMutex};
    for (auto message = messages.begin(); message != messages.end();)
    {
        if (message->finished())
        {
            message = messages.erase(message);
            continue;
        }
        message->update(currentY);

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
    for (auto &message : messages) { message.render(); }
}

void ui::PopMessageManager::push_message(int displayTicks, std::string_view message)
{
    PopMessageManager &manager = PopMessageManager::get_instance();
    std::mutex &queueMutex     = manager.m_queueMutex;
    std::mutex &messageMutex   = manager.m_messageMutex;
    auto &messageQueue         = manager.m_messageQueue;
    auto &messages             = manager.m_messages;

    {
        std::lock_guard messageGuard{messageMutex};
        if (!messages.empty())
        {
            ui::PopMessage &back               = messages.back();
            const std::string_view lastMessage = back.get_message();
            if (lastMessage == message) { return; }
        }
    }

    std::lock_guard queueGuard(queueMutex);
    auto queuePair = std::make_pair(displayTicks, std::string{message});
    messageQueue.push_back(std::move(queuePair));
}

void ui::PopMessageManager::push_message(int displayTicks, std::string &message)
{
    PopMessageManager &manager = PopMessageManager::get_instance();
    std::mutex &queueMutex     = manager.m_queueMutex;
    std::mutex &messageMutex   = manager.m_messageMutex;
    auto &messageQueue         = manager.m_messageQueue;
    auto &messages             = manager.m_messages;

    {
        std::lock_guard messageGuard{messageMutex};
        if (!messages.empty())
        {
            ui::PopMessage &back               = messages.back();
            const std::string_view lastMessage = back.get_message();
            if (lastMessage == message) { return; }
        }
    }

    std::lock_guard queueGuard(queueMutex);
    auto queuePair = std::make_pair(displayTicks, std::move(message));
    messageQueue.push_back(std::move(queuePair));
}

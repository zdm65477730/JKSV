#pragma once
#include "system/Timer.hpp"
#include <mutex>
#include <string>
#include <vector>

namespace ui
{
    // This is the actual struct containing data for a message.
    typedef struct
    {
            // Y coordinate
            double m_y;
            // Target Y coordinate
            double m_targetY;
            // Width of the message.
            size_t m_width;
            // Message string.
            std::string m_message;
            // Timer.
            sys::Timer m_timer;
    } PopMessage;

    class PopMessageManager
    {
        public:
            // No copying.
            PopMessageManager(const PopMessageManager &) = delete;
            PopMessageManager(PopMessageManager &&) = delete;
            PopMessageManager &operator=(const PopMessageManager &) = delete;
            PopMessageManager &operator=(PopMessageManager &&) = delete;

            /// @brief Updates and processes message queue.
            static void update();

            /// @brief Renders messages to screen.
            static void render();

            /// @brief Pushes a new message to the queue for processing.
            /// @param displayTicks Number of ticks for the message to be displayed until it is purged.
            /// @param format Format of message.
            /// @param args Arguments for message.
            static void push_message(int displayTicks, const char *format, ...);

            /// @brief The default duration of ticks for messages to be shown.
            static constexpr int DEFAULT_MESSAGE_TICKS = 2500;

        private:
            // Only one instance allowed.
            PopMessageManager() = default;
            // Returns the only instance.
            static PopMessageManager &get_instance()
            {
                static PopMessageManager manager;
                return manager;
            }
            // The queue for processing. SDL can't handle things being rendered in multiple threads.
            std::vector<std::pair<int, std::string>> m_messageQueue;
            // Actual vector of messages
            std::vector<ui::PopMessage> m_messages;
            // Mutex to attempt to make this thread safe.
            std::mutex m_messageMutex;
    };
} // namespace ui

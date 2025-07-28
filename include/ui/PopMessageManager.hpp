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
            double y{};
            // Target Y coordinate
            double targetY{};
            // Width of the message.
            size_t width{};
            // Message string.
            std::string message{};
            // Timer.
            sys::Timer timer{};
    } PopMessage;

    class PopMessageManager
    {
        public:
            // No copying.
            PopMessageManager(const PopMessageManager &)            = delete;
            PopMessageManager(PopMessageManager &&)                 = delete;
            PopMessageManager &operator=(const PopMessageManager &) = delete;
            PopMessageManager &operator=(PopMessageManager &&)      = delete;

            /// @brief Updates and processes message queue.
            static void update();

            /// @brief Renders messages to screen.
            static void render();

            /// @brief Pushes a new message to the queue for processing.
            static void push_message(int displayTicks, std::string_view message);

            /// @brief The default duration of ticks for messages to be shown.
            static constexpr int DEFAULT_TICKS = 2500;

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
            std::vector<std::pair<int, std::string>> m_messageQueue{};

            // Actual vector of messages
            std::vector<ui::PopMessage> m_messages{};

            // Mutex to attempt to make this thread safe.
            std::mutex m_messageMutex{};

            std::mutex m_queueMutex{};
    };
} // namespace ui

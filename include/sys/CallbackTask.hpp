#pragma once
#include "sys/Task.hpp"

#include <atomic>

namespace sys
{
    class CallbackTask final : public sys::Task
    {
        public:
            template <typename... Args>
            CallbackTask(void (*function)(sys::CallbackTask *, Args...), Args... args)
            {
                m_thread = std::thread(function, this, std::forward<Args>(args)...);
            }

            /// @brief Signals that the main task is finished.
            void task_complete();

            /// @brief
            void callback_complete();

            /// @brief Returns whether both the main task and callback have finished.
            bool is_running() const;

        private:
            /// @brief Bool to signal whether or not the task thread is completed.
            std::atomic<bool> m_taskFinished{};

            /// @brief This signals whether or not the callback is finished.s
            bool m_callbackFinished{};
    };
}

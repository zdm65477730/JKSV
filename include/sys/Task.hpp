#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// This macro helps keep things a bit easier to read and cuts down on repetition.
#define TASK_FINISH_RETURN(x)                                                                                                  \
    x->complete();                                                                                                             \
    return

namespace sys
{
    /// @brief Class that runs tasks in a thread and automatically deactivates when finished.
    class Task
    {
        public:
            /// @brief Default constructor.
            Task() = default;

            template <typename... Args>
            Task(void (*function)(sys::Task *, Args...), Args... args)
            {
                m_thread = std::thread(function, this, std::forward<Args>(args)...);
                m_isRunning.store(true);
            }

            /// @brief Required destructor.
            virtual ~Task();

            /// @brief Returns if the thread has signaled it's finished running.
            /// @return True if the thread is still running. False if it isn't.
            bool is_running() const;

            /// @brief Allows thread to signal it's finished.
            /// @note Spawned task threads must call this when their work is finished.
            void complete();

            /// @brief Sets the task/threads current status string. Thread safe.
            void set_status(std::string_view status);

            /// @brief Returns the status string. Thread safe.
            /// @return Copy of the status string.
            std::string get_status();

        protected:
            // Whether task is still running.
            std::atomic<bool> m_isRunning{};

            // Thread
            std::thread m_thread{};

        private:
            // Status string the thread can set that the main thread can display.
            std::string m_status{};

            // Mutex so that string doesn't get messed up.
            std::mutex m_statusLock{};
    };
} // namespace sys

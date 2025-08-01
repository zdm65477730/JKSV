#pragma once
#include <mutex>
#include <string>
#include <thread>

// This macro helps keep things a bit easier to read and cuts down on repetition.
#define TASK_FINISH_RETURN(x)                                                                                                  \
    x->finished();                                                                                                             \
    return

namespace sys
{
    /// @brief Class that runs tasks in a thread and automatically deactivates when finished.
    class Task
    {
        public:
            /// @brief Constructs a new task.
            /// @param function Function for task to run.
            /// @param args Arguments forwarded to thread.
            /// @note Functions passed to this class must follow the following signature: void function(sys::Task *,
            /// <arguments>)
            template <typename... Args>
            Task(void (*function)(sys::Task *, Args...), Args... args)
            {
                m_thread = std::thread(function, this, std::forward<Args>(args)...);
            }

            /// @brief Alternate version of the above that allows derived classes to pass themselves to the thread instead.
            /// @tparam TaskType Type of task passed to the spawned thread. Ex: ProgressTask instead of Task.
            /// @param function Function for task to run.
            /// @param task Task passed to function. You don't really need to worry about this.
            /// @param args Arguments to forward to the function.
            template <typename TaskType, typename... Args>
            Task(void (*function)(TaskType *, Args...), TaskType *task, Args... args)
            {
                m_thread = std::thread(function, task, std::forward<Args>(args)...);
            }

            /// @brief Required destructor.
            virtual ~Task();

            /// @brief Returns if the thread has signaled it's finished running.
            /// @return True if the thread is still running. False if it isn't.
            bool is_running() const;

            /// @brief Allows thread to signal it's finished.
            /// @note Spawned task threads must call this when their work is finished.
            void finished();

            /// @brief Sets the task/threads current status string. Thread safe.
            void set_status(std::string_view status);

            /// @brief Returns the status string. Thread safe.
            /// @return Copy of the status string.
            std::string get_status();

        private:
            // Whether task is still running.
            bool m_isRunning = true;
            // Status string the thread can set that the main thread can display.
            std::string m_status;
            // Mutex so that string doesn't get messed up.
            std::mutex m_statusLock;
            // Thread
            std::thread m_thread;
    };
} // namespace sys

#include "sys/threadpool.hpp"

#include "error.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <switch.h>

namespace
{
    /// @brief Number of threads to pool.
    constexpr size_t COUNT_THREADS = 2;

    /// @brief Size of the stack for the threads.
    constexpr size_t SIZE_THREAD_STACK = 0x40000;

    /// @brief Another time/hand saver.
    using QueuePair = std::pair<sys::threadpool::JobFunction, sys::threadpool::JobData>;

    /// @brief Array of threads.
    constinit std::array<Thread, COUNT_THREADS> s_threads{};

    /// @brief The queue of jobs.
    std::queue<QueuePair> s_jobQueue{};

    /// @brief Mutex to prevent job corruption.
    std::mutex s_jobMutex{};

    /// @brief Conditional.
    std::condition_variable s_jobCondition{};

    /// @brief So exit can signal.
    std::atomic_bool s_exitFlag{};
}

/// @brief Defined at bottom.
static void thread_pool_function(void *);

void sys::threadpool::initialize()
{
    for (size_t i = 0; i < COUNT_THREADS; i++)
    {
        // NOTE: If pool size increases, i + 1 isn't going to work anymore.
        error::libnx(threadCreate(&s_threads[i], thread_pool_function, nullptr, nullptr, SIZE_THREAD_STACK, 0x2C, i + 1));
        error::libnx(threadStart(&s_threads[i]));
    }
}

void sys::threadpool::exit()
{
    s_exitFlag.store(true);
    s_jobCondition.notify_all();
    for (size_t i = 0; i < COUNT_THREADS; i++)
    {
        error::libnx(threadWaitForExit(&s_threads[i]));
        error::libnx(threadClose(&s_threads[i]));
    }
}

void sys::threadpool::push_job(sys::threadpool::JobFunction function, sys::threadpool::JobData data)
{
    std::lock_guard jobGuard{s_jobMutex};
    s_jobQueue.push(std::make_pair(function, data));
    s_jobCondition.notify_one();
}

static void thread_pool_function(void *)
{
    auto condition = []() { return s_exitFlag.load() || !s_jobQueue.empty(); };

    while (true)
    {
        std::unique_lock jobGuard{s_jobMutex};
        s_jobCondition.wait(jobGuard, condition);

        if (s_exitFlag.load()) { break; }

        auto [function, data] = s_jobQueue.front();
        s_jobQueue.pop();
        jobGuard.unlock();

        function(data);
    }
}
#pragma once
#include "sys/defines.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

class BufferQueue final
{
    public:
        /// @brief Makes thing easier to read and type.
        using Buffer = std::unique_ptr<sys::Byte[]>;

        /// @brief This is a pair in the queue.
        using QueuePair = std::pair<Buffer, size_t>;

        /// @brief Less of a pain to type out.
        using BufferGuard = std::lock_guard<std::mutex>;

        /// @brief Creates a new BufferQueue.
        /// @param bufferLimit Number of buffers before the queue is considered full.
        BufferQueue(int bufferLimit) noexcept
            : m_bufferLimit(bufferLimit) {};

        inline bool try_push(Buffer &buffer, size_t bufferSize)
        {
            std::lock_guard queueGuard{m_queueMutex};
            if (m_bufferLimit != NO_LIMIT && static_cast<int>(m_queue.size()) >= m_bufferLimit) { return false; }

            m_queue.emplace(std::move(buffer), bufferSize);
            return true;
        }

        inline bool get_front(QueuePair &pairOut)
        {
            std::lock_guard queueGuard{m_queueMutex};
            if (m_queue.empty()) { return false; }

            pairOut = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        static inline void default_delay() noexcept { std::this_thread::sleep_for(std::chrono::microseconds(10)); }

        /// @brief This can be passed to the constructor to make is_full() return false all of the time.
        static inline int NO_LIMIT = -1;

    private:
        /// @brief Maximum number of buffers.
        int m_bufferLimit{};

        /// @brief Stores the size of the buffers allocated with allocate_buffer.
        size_t m_bufferSize{};

        /// @brief Mutex to lock the queue with.
        std::mutex m_queueMutex{};

        /// @brief Queue of buffers.
        std::queue<QueuePair> m_queue{};
};

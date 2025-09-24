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
        BufferQueue(int bufferLimit, size_t bufferSize) noexcept
            : m_bufferLimit(bufferLimit)
            , m_bufferSize(bufferSize) {};

        /// @brief Gets an returns a scoped lock_guard for the internal mutex.
        inline BufferGuard lock_queue() noexcept { return BufferGuard{m_queueMutex}; }

        /// @brief Returns if the buffer queue is "full", or, has reached its limit.
        inline bool is_full() const noexcept
        {
            if (static_cast<int>(m_queue.size()) < m_bufferLimit) { return false; }

            std::this_thread::sleep_for(std::chrono::microseconds(10));
            return true;
        }

        /// @brief Returns if the buffer queue is empty.
        inline bool is_empty() const noexcept
        {
            if (!m_queue.empty()) { return false; }

            std::this_thread::sleep_for(std::chrono::microseconds(10));
            return true;
        }

        /// @brief Creates and returns a buffer of bufferSize.
        inline Buffer allocate_buffer() { return std::make_unique<sys::Byte[]>(m_bufferSize); }

        /// @brief Same as above, but with the size passed here.
        inline Buffer allocate_buffer(size_t bufferSize) { return std::make_unique<sys::Byte[]>(bufferSize); }

        /// @brief Pushes a new buffer to the queue.
        inline void push_to_queue(Buffer &buffer, size_t bufferSize)
        {
            auto queuePair = std::make_pair(std::move(buffer), bufferSize);
            m_queue.push(std::move(queuePair));
        }

        /// @brief Returns the front of the queue and pops it.
        inline QueuePair get_front()
        {
            auto front = std::move(m_queue.front());
            m_queue.pop();
            return front;
        }

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

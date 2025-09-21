#pragma once
#include "fslib.hpp"
#include "sys/sys.hpp"

#include <array>
#include <condition_variable>
#include <mutex>
#include <semaphore>
#include <vector>

namespace curl
{
    inline constexpr size_t SHARED_BUFFER_SIZE = 0x500000;

    // This is for synchronizing the buffer.
    enum class BufferState
    {
        Empty,
        Full
    };

    // clang-format off
    struct DownloadStruct : sys::threadpool::DataStruct
    {
        /// @brief Buffer mutex.
        std::mutex lock{};

        /// @brief Conditional for when the buffer is full.
        std::condition_variable condition{};

        /// @brief Shared buffer that is read into.
        std::vector<sys::Byte> sharedBuffer{};

        /// @brief Bool to signal when the buffer is ready/empty.
        BufferState bufferState{};

        /// @brief Destination file to write to.
        fslib::File *dest{};

        /// @brief Optional. Task to update with progress.
        sys::ProgressTask *task{};

        /// @brief Current offset in the file.
        size_t offset{};
        
        /// @brief Size of the file being downloaded.
        int64_t fileSize{};

        /// @brief Signals when the write thread is complete.
        std::binary_semaphore writeComplete{0};
    };
    // clang-format on

    static inline std::shared_ptr<curl::DownloadStruct> create_download_struct(fslib::File &dest,
                                                                               sys::ProgressTask *task,
                                                                               int64_t fileSize)
    {
        auto downloadStruct      = std::make_shared<curl::DownloadStruct>();
        downloadStruct->dest     = &dest;
        downloadStruct->task     = task;
        downloadStruct->fileSize = fileSize;
        downloadStruct->sharedBuffer.reserve(SHARED_BUFFER_SIZE);
        return downloadStruct;
    }
}

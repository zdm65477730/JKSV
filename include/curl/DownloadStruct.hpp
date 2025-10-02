#pragma once
#include "BufferQueue.hpp"
#include "fslib.hpp"
#include "sys/sys.hpp"

#include <array>
#include <mutex>
#include <queue>
#include <semaphore>

namespace curl
{
    inline constexpr int DOWNLOAD_QUEUE_LIMIT = 128;

    // clang-format off
    struct DownloadStruct : sys::threadpool::DataStruct
    {
        /// @brief Buffer queue for downloads.
        BufferQueue bufferQueue{BufferQueue::NO_LIMIT};

        /// @brief Destination file to write to.
        fslib::File *dest{};

        /// @brief Optional. Task to update with progress.
        sys::ProgressTask *task{};

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
        return downloadStruct;
    }
}

#pragma once
#include "fslib.hpp"
#include "sys/sys.hpp"

#include <condition_variable>
#include <mutex>
#include <vector>

namespace curl
{
    // clang-format off
    struct DownloadStruct
    {
        /// @brief Buffer mutex.
        std::mutex lock{};

        /// @brief Conditional for when the buffer is full.
        std::condition_variable condition{};

        /// @brief Shared buffer that is read into.
        std::vector<sys::byte> sharedBuffer{};

        /// @brief Bool to signal when the buffer is ready/empty.
        bool bufferReady{};

        /// @brief Destination file to write to.
        fslib::File *dest{};

        /// @brief Optional. Task to update with progress.
        sys::ProgressTask *task{};

        /// @brief Current offset in the file.
        size_t offset{};
        
        /// @brief Size of the file being downloaded.
        int64_t fileSize{};
    };
    // clang-format on
}

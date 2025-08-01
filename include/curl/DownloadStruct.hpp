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
        std::mutex lock{};
        std::condition_variable condition{};
        std::vector<sys::byte> sharedBuffer{};
        bool bufferReady{};
        fslib::File *dest{};
        sys::ProgressTask *task{};
        size_t offset{};
        int64_t fileSize{};
    };
    // clang-format on
}

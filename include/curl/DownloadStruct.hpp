#pragma once
#include "fslib.hpp"
#include "system/ProgressTask.hpp"
#include "system/defines.hpp"

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
        std::vector<byte> sharedBuffer{};
        bool bufferReady{};
        fslib::File *dest{};
        sys::ProgressTask *task{};
        size_t offset{};
        size_t fileSize{};
    };
    // clang-format on
}

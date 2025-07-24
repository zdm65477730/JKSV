#pragma once
#include "fslib.hpp"
#include "system/ProgressTask.hpp"

namespace curl
{
    // clang-format off
    struct UploadStruct
    {
        fslib::File *source{};
        sys::ProgressTask *task{};
    };
    // clang-format on
}

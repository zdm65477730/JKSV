#pragma once
#include "fslib.hpp"
#include "sys/sys.hpp"

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

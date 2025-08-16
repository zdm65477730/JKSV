#pragma once
#include "fslib.hpp"
#include "sys/sys.hpp"

namespace curl
{
    // clang-format off
    struct UploadStruct
    {
        /// @brief Source file to upload from.
        fslib::File *source{};
        /// @brief Optional. Task to update with progress.
        sys::ProgressTask *task{};
    };
    // clang-format on
}

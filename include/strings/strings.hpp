#pragma once
#include "strings/names.hpp"

#include <string_view>

namespace strings
{
    // Attempts to load strings from file in RomFS.
    bool initialize();

    // Returns string with name and index. Returns nullptr if string doesn't exist.
    const char *get_by_name(std::string_view name, int index);
} // namespace strings

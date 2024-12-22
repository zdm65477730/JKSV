#pragma once
#include <string_view>
#include <switch.h>

namespace Keyboard
{
    bool GetInput(SwkbdType KeyboardType, std::string_view DefaultText, std::string_view Header, char *StringOut, size_t StringLength);
} // namespace Keyboard

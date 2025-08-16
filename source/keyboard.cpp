#include "keyboard.hpp"

#include "logging/error.hpp"

#include <string>

bool keyboard::get_input(SwkbdType keyboardType,
                         std::string_view defaultText,
                         std::string_view header,
                         char *stringOut,
                         size_t stringLength)
{
    SwkbdConfig keyboard;
    swkbdCreate(&keyboard, 0);
    swkbdConfigSetBlurBackground(&keyboard, true);
    swkbdConfigSetInitialText(&keyboard, defaultText.data());
    swkbdConfigSetHeaderText(&keyboard, header.data());
    swkbdConfigSetGuideText(&keyboard, header.data());
    swkbdConfigSetType(&keyboard, keyboardType);
    swkbdConfigSetStringLenMax(&keyboard, stringLength);
    swkbdConfigSetKeySetDisableBitmask(&keyboard, SwkbdKeyDisableBitmask_ForwardSlash | SwkbdKeyDisableBitmask_Backslash);

    const bool swkbdError = error::libnx(swkbdShow(&keyboard, stringOut, stringLength));
    const bool empty      = std::char_traits<char>::length(stringOut) == 0;
    if (swkbdError || empty) { return false; }

    return true;
}

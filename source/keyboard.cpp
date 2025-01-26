#include "keyboard.hpp"
#include <string>

bool keyboard::getInput(SwkbdType keyboardType, std::string_view defaultText, std::string_view header, char *stringOut, size_t stringLength)
{
    // Setup keyboard.
    SwkbdConfig keyboard;
    swkbdCreate(&keyboard, 0); // Old JKSV actually used dictionary words, but I don't feel like implementing them again.
    swkbdConfigSetBlurBackground(&keyboard, true);
    swkbdConfigSetInitialText(&keyboard, defaultText.data());
    swkbdConfigSetHeaderText(&keyboard, header.data());
    swkbdConfigSetGuideText(&keyboard, header.data());
    swkbdConfigSetType(&keyboard, keyboardType);
    swkbdConfigSetStringLenMax(&keyboard, stringLength);
    swkbdConfigSetKeySetDisableBitmask(&keyboard, SwkbdKeyDisableBitmask_ForwardSlash | SwkbdKeyDisableBitmask_Backslash);

    // If it fails, just return.
    if (R_FAILED(swkbdShow(&keyboard, stringOut, stringLength)))
    {
        return false;
    }

    // If the string is empty, assume failure or cancel.
    if (std::char_traits<char>::length(stringOut) == 0)
    {
        return false;
    }

    // I wish this was more like the 3DS keyboard cause that actually returned what button was pressed...
    return true;
}

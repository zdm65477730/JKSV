#include "Keyboard.hpp"
#include <string>

bool Keyboard::GetInput(SwkbdType KeyboardType, std::string_view DefaultText, std::string_view Header, char *StringOut, size_t StringLength)
{
    // Setup keyboard.
    SwkbdConfig Keyboard;
    swkbdCreate(&Keyboard, 0); // Old JKSV actually used dictionary words, but I don't feel like implementing them again.
    swkbdConfigSetBlurBackground(&Keyboard, true);
    swkbdConfigSetInitialText(&Keyboard, DefaultText.data());
    swkbdConfigSetHeaderText(&Keyboard, Header.data());
    swkbdConfigSetGuideText(&Keyboard, Header.data());
    swkbdConfigSetType(&Keyboard, KeyboardType);
    swkbdConfigSetStringLenMax(&Keyboard, StringLength);
    swkbdConfigSetKeySetDisableBitmask(&Keyboard, SwkbdKeyDisableBitmask_ForwardSlash | SwkbdKeyDisableBitmask_Backslash);

    // If it fails, just return.
    if (R_FAILED(swkbdShow(&Keyboard, StringOut, StringLength)))
    {
        return false;
    }

    // If the string is empty, assume failure or cancel.
    if (std::char_traits<char>::length(StringOut) == 0)
    {
        return false;
    }

    // I wish this was more like the 3DS keyboard cause that actually returned what button was pressed...
    return true;
}

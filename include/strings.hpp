#pragma once
#include <string_view>

namespace strings
{
    // Attempts to load strings from file in RomFS.
    bool initialize(void);
    // Returns string with name and index. Returns nullptr if string doesn't exist.
    const char *getByName(std::string_view name, int index);
    // Names of strings to prevent typos.
    namespace names
    {
        static constexpr std::string_view TRANSLATION_INFO = "TranslationInfo";
        static constexpr std::string_view CONTROL_GUIDES = "ControlGuides";
        static constexpr std::string_view SAVE_DATA_TYPES = "SaveDataTypes";
        static constexpr std::string_view MAIN_MENU_NAMES = "MainMenuNames";
        static constexpr std::string_view SETTINGS_MENU = "SettingsMenu";
        static constexpr std::string_view EXTRAS_MENU = "ExtrasMenu";
        static constexpr std::string_view YES_NO = "YesNo";
        static constexpr std::string_view HOLDING_STRINGS = "HoldingStrings";
        static constexpr std::string_view ON_OFF = "OnOff";
        static constexpr std::string_view BACKUP_MENU = "BackupMenu";
        static constexpr std::string_view COPYING_FILES = "CopyingFiles";
        static constexpr std::string_view BACKUPMENU_CONFIRMATIONS = "BackupMenuConfirmations";
        static constexpr std::string_view DELETING_FILES = "DeletingFiles";
        static constexpr std::string_view KEYBOARD_STRINGS = "KeyboardStrings";
        static constexpr std::string_view USER_OPTIONS = "UserOptions";
        static constexpr std::string_view CREATING_SAVE_DATA_FOR = "CreatingSaveDataFor";
        static constexpr std::string_view POP_MESSAGES = "PopMessages";
    } // namespace names
} // namespace strings

#pragma once
#include <string_view>

namespace strings
{
    // Attempts to load strings from file in RomFS.
    bool initialize();

    // Returns string with name and index. Returns nullptr if string doesn't exist.
    const char *get_by_name(std::string_view name, int index);

    // Names of strings to prevent typos.
    namespace names
    {
        static constexpr std::string_view BACKUPMENU_MENU       = "BackupMenu";
        static constexpr std::string_view BACKUPMENU_CONFS      = "BackupMenuConfirmations";
        static constexpr std::string_view BACKUPMENU_POPS       = "BackupMenuPops";
        static constexpr std::string_view BACKUPMENU_STATUS     = "BackupMenuStatus";
        static constexpr std::string_view CONTROL_GUIDES        = "ControlGuides";
        static constexpr std::string_view EXTRASMENU_MENU       = "ExtrasMenu";
        static constexpr std::string_view EXTRASMENU_POPS       = "ExtrasPops";
        static constexpr std::string_view GENERAL_POPS          = "GeneralPops";
        static constexpr std::string_view GOOGLE_DRIVE          = "GoogleDriveStrings";
        static constexpr std::string_view HOLDING_STRINGS       = "HoldingStrings";
        static constexpr std::string_view IO_STATUSES           = "IOStatuses";
        static constexpr std::string_view IO_POPS               = "IOPops";
        static constexpr std::string_view KEYBOARD              = "KeyboardStrings";
        static constexpr std::string_view MAINMENU_CONFS        = "MainMenuConfs";
        static constexpr std::string_view MAINMENU_POPS         = "MainMenuPops";
        static constexpr std::string_view ON_OFF                = "OnOff";
        static constexpr std::string_view REMOTE_POPS           = "RemotePops";
        static constexpr std::string_view SAVECREATE_POPS       = "SaveCreatePops";
        static constexpr std::string_view SAVE_DATA_TYPES       = "SaveDataTypes";
        static constexpr std::string_view SETTINGS_DESCRIPTIONS = "SettingsDescriptions";
        static constexpr std::string_view SETTINGS_MENU         = "SettingsMenu";
        static constexpr std::string_view SORT_TYPES            = "SortTypes";
        static constexpr std::string_view TITLEINFO             = "TitleInfo";
        static constexpr std::string_view TITLEOPTION_CONFS     = "TitleOptionConfirmations";
        static constexpr std::string_view TITLEOPTION_POPS      = "TitleOptionPops";
        static constexpr std::string_view TITLEOPTION_STATUS    = "TitleOptionStatus";
        static constexpr std::string_view TITLEOPTION           = "TitleOptions";
        static constexpr std::string_view TRANSLATION           = "TranslationInfo";
        static constexpr std::string_view USEROPTION_CONFS      = "UserOptionConfirmations";
        static constexpr std::string_view USEROPTION_STATUS     = "UserOptionStatus";
        static constexpr std::string_view USEROPTION_MENU       = "UserOptions";
        static constexpr std::string_view WEBDAV                = "WebDavStrings";
        static constexpr std::string_view YES_NO                = "YesNo";
    } // namespace names
} // namespace strings

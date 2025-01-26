#pragma once
#include "fslib.hpp"
#include <string_view>

namespace config
{
    /// @brief Attempts to load config from file. If it fails, loads defaults.
    void initialize(void);

    /// @brief Resets config to default values.
    void resetToDefault(void);

    /// @brief Saves config to file.
    void save(void);

    /// @brief Retrieves the config value according to the key passed.
    /// @param key Key to retrieve. See config::keys
    /// @return Key's value if found. 0 if it is not.
    uint8_t getByKey(std::string_view key);

    /// @brief Retrieves value of config at index.
    /// @param index Index of value to retrieve.
    uint8_t getByIndex(int index);

    /// @brief Returns the working directory.
    /// @return Working directory.
    fslib::Path getWorkingDirectory(void);

    /// @brief Returns the scaling speed of UI transitions and animations.
    /// @return Scaling variable.
    double getAnimationScaling(void);

    /// @brief Adds or removes a title from the favorites list.
    /// @param applicationID Application ID of title to add or remove.
    void addRemoveFavorite(uint64_t applicationID);

    /// @brief Returns if the title is found in the favorites list.
    /// @param applicationID Application ID to search for.
    /// @return True if found. False if not.
    bool isFavorite(uint64_t applicationID);

    /// @brief Adds or removes title from blacklist.
    /// @param applicationID Application ID to add or remove.
    void addRemoveBlacklist(uint64_t applicationID);

    /// @brief Returns if the title is found in the blacklist.
    /// @param applicationID Application ID to search for.
    /// @return True if found. False if not.
    bool isBlacklisted(uint64_t applicationID);

    // Names of keys. Note: Not all of these are retrievable with GetByKey. Some of these are purely for config reading and writing.
    namespace keys
    {
        static constexpr std::string_view WORKING_DIRECTORY = "WorkingDirectory";
        static constexpr std::string_view INCLUDE_DEVICE_SAVES = "IncludeDeviceSaves";
        static constexpr std::string_view AUTO_BACKUP_ON_RESTORE = "AutoBackupOnRestore";
        static constexpr std::string_view AUTO_NAME_BACKUPS = "AutoNameBackups";
        static constexpr std::string_view AUTO_UPLOAD = "AutoUploadToRemote";
        static constexpr std::string_view HOLD_FOR_DELETION = "HoldForDeletion";
        static constexpr std::string_view HOLD_FOR_RESTORATION = "HoldForRestoration";
        static constexpr std::string_view HOLD_FOR_OVERWRITE = "HoldForOverWrite";
        static constexpr std::string_view ONLY_LIST_MOUNTABLE = "OnlyListMountable";
        static constexpr std::string_view LIST_ACCOUNT_SYS_SAVES = "ListAccountSystemSaves";
        static constexpr std::string_view ALLOW_WRITING_TO_SYSTEM = "AllowSystemSaveWriting";
        static constexpr std::string_view EXPORT_TO_ZIP = "ExportToZip";
        static constexpr std::string_view ZIP_COMPRESSION_LEVEL = "ZipCompressionLevel";
        static constexpr std::string_view TITLE_SORT_TYPE = "TitleSortType";
        static constexpr std::string_view JKSM_TEXT_MODE = "JKSMTextMode";
        static constexpr std::string_view FORCE_ENGLISH = "ForceEnglish";
        static constexpr std::string_view ENABLE_TRASH_BIN = "EnableTrash";
        static constexpr std::string_view UI_ANIMATION_SCALE = "UIAnimationScaling";
        static constexpr std::string_view FAVORITES = "Favorites";
        static constexpr std::string_view BLACKLIST = "BlackList";
    } // namespace keys
} // namespace config

#include "config.hpp"
#include "JSON.hpp"
#include "logger.hpp"
#include "stringutil.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace
{
    /// @brief This is the default working directory path.
    constexpr std::string_view PATH_DEFAULT_WORK_DIR = "sdmc:/JKSV";
    // Folder path.
    constexpr std::string_view PATH_CONFIG_FOLDER = "sdmc:/config/JKSV";
    // Paths file. Funny name too.
    constexpr std::string_view PATH_PATHS_PATH = "sdmc:/config/JKSV/Paths.json";
    // Actual config path.
    const char *PATH_CONFIG_FILE = "sdmc:/config/JKSV/JKSV.json";

    /// @brief This map holds the majority of config values.
    std::map<std::string, uint8_t> s_configMap;

    // Working directory
    fslib::Path s_workingDirectory;
    // UI animation scaling.
    double s_uiAnimationScaling;
    // Vector of favorite title ids
    std::vector<uint64_t> s_favorites;
    // Vector of titles to ignore.
    std::vector<uint64_t> s_blacklist;

    // Map of paths.
    std::map<uint64_t, std::string> s_pathMap;
} // namespace

static void read_array_to_vector(std::vector<uint64_t> &vector, json_object *array)
{
    // Just in case. Shouldn't happen though.
    vector.clear();

    size_t arrayLength = json_object_array_length(array);
    for (size_t i = 0; i < arrayLength; i++)
    {
        json_object *arrayEntry = json_object_array_get_idx(array, i);
        if (!arrayEntry)
        {
            continue;
        }
        vector.push_back(std::strtoull(json_object_get_string(arrayEntry), NULL, 16));
    }
}

void config::initialize()
{
    if (!fslib::directory_exists(PATH_CONFIG_FOLDER) && !fslib::create_directories_recursively(PATH_CONFIG_FOLDER))
    {
        logger::log("Error creating config folder: %s.", fslib::get_error_string());
        config::reset_to_default();
        return;
    }

    json::Object configJSON = json::new_object(json_object_from_file, PATH_CONFIG_FILE);
    if (!configJSON)
    {
        logger::log("Error opening config for reading: %s", fslib::get_error_string());
        config::reset_to_default();
        return;
    }

    json_object_iterator configIterator = json_object_iter_begin(configJSON.get());
    json_object_iterator configEnd = json_object_iter_end(configJSON.get());
    while (!json_object_iter_equal(&configIterator, &configEnd))
    {
        const char *keyName = json_object_iter_peek_name(&configIterator);
        json_object *configValue = json_object_iter_peek_value(&configIterator);

        // These are exemptions.
        if (std::strcmp(keyName, config::keys::WORKING_DIRECTORY.data()) == 0)
        {
            s_workingDirectory = json_object_get_string(configValue);
        }
        else if (std::strcmp(keyName, config::keys::UI_ANIMATION_SCALE.data()) == 0)
        {
            s_uiAnimationScaling = json_object_get_double(configValue);
        }
        else if (std::strcmp(keyName, config::keys::FAVORITES.data()) == 0)
        {
            read_array_to_vector(s_favorites, configValue);
        }
        else if (std::strcmp(keyName, config::keys::BLACKLIST.data()) == 0)
        {
            read_array_to_vector(s_blacklist, configValue);
        }
        else
        {
            s_configMap[keyName] = json_object_get_uint64(configValue);
        }
        json_object_iter_next(&configIterator);
    }

    // Load custom output paths.
    if (!fslib::file_exists(PATH_PATHS_PATH))
    {
        // Just bail.
        return;
    }

    json::Object pathsJSON = json::new_object(json_object_from_file, PATH_PATHS_PATH.data());
    if (!pathsJSON)
    {
        return;
    }

    json_object_iterator pathsIterator = json_object_iter_begin(pathsJSON.get());
    json_object_iterator pathsEnd = json_object_iter_end(pathsJSON.get());
    while (!json_object_iter_equal(&pathsIterator, &pathsEnd))
    {
        // Grab these
        uint64_t applicationID = std::strtoull(json_object_iter_peek_name(&pathsIterator), NULL, 16);
        json_object *path = json_object_iter_peek_value(&pathsIterator);

        // Map em.
        s_pathMap[applicationID] = json_object_get_string(path);

        json_object_iter_next(&pathsIterator);
    }
}

void config::reset_to_default()
{
    s_workingDirectory = PATH_DEFAULT_WORK_DIR;
    s_configMap[config::keys::INCLUDE_DEVICE_SAVES.data()] = 0;
    s_configMap[config::keys::AUTO_BACKUP_ON_RESTORE.data()] = 1;
    s_configMap[config::keys::AUTO_NAME_BACKUPS.data()] = 0;
    s_configMap[config::keys::AUTO_UPLOAD.data()] = 0;
    s_configMap[config::keys::HOLD_FOR_DELETION.data()] = 1;
    s_configMap[config::keys::HOLD_FOR_RESTORATION.data()] = 1;
    s_configMap[config::keys::HOLD_FOR_OVERWRITE.data()] = 1;
    s_configMap[config::keys::ONLY_LIST_MOUNTABLE.data()] = 1;
    s_configMap[config::keys::LIST_ACCOUNT_SYS_SAVES.data()] = 0;
    s_configMap[config::keys::ALLOW_WRITING_TO_SYSTEM.data()] = 0;
    s_configMap[config::keys::EXPORT_TO_ZIP.data()] = 1;
    s_configMap[config::keys::ZIP_COMPRESSION_LEVEL.data()] = 6;
    s_configMap[config::keys::TITLE_SORT_TYPE.data()] = 0;
    s_configMap[config::keys::JKSM_TEXT_MODE.data()] = 0;
    s_configMap[config::keys::FORCE_ENGLISH.data()] = 0;
    s_configMap[config::keys::ENABLE_TRASH_BIN.data()] = 0;
    s_uiAnimationScaling = 2.5f;
}

void config::save()
{
    {
        json::Object configJSON = json::new_object(json_object_new_object);

        // Add working directory first.
        json_object *workingDirectory = json_object_new_string(s_workingDirectory.c_string());
        json::add_object(configJSON, config::keys::WORKING_DIRECTORY.data(), workingDirectory);

        // Loop through map and add it.
        for (auto &[key, value] : s_configMap)
        {
            json_object *jsonValue = json_object_new_uint64(value);
            json::add_object(configJSON, key.c_str(), jsonValue);
        }

        // Add UI scaling.
        json_object *scaling = json_object_new_double(s_uiAnimationScaling);
        json::add_object(configJSON, config::keys::UI_ANIMATION_SCALE.data(), scaling);

        // Favorites
        json_object *favoritesArray = json_object_new_array();
        for (uint64_t &titleID : s_favorites)
        {
            // Need to do it like this or json-c does decimal instead of hex.
            json_object *newFavorite =
                json_object_new_string(stringutil::get_formatted_string("%016lX", titleID).c_str());
            json_object_array_add(favoritesArray, newFavorite);
        }
        json::add_object(configJSON, config::keys::FAVORITES.data(), favoritesArray);

        // Same but blacklist
        json_object *blacklistArray = json_object_new_array();
        for (uint64_t &titleID : s_blacklist)
        {
            json_object *newBlacklist =
                json_object_new_string(stringutil::get_formatted_string("%016lX", titleID).c_str());
            json_object_array_add(blacklistArray, newBlacklist);
        }
        json::add_object(configJSON, config::keys::BLACKLIST.data(), blacklistArray);

        // Write config file
        fslib::File configFile(PATH_CONFIG_FILE,
                               FsOpenMode_Create | FsOpenMode_Write,
                               std::strlen(json_object_get_string(configJSON.get())));
        if (configFile)
        {
            configFile << json_object_get_string(configJSON.get());
        }
    }

    if (!s_pathMap.empty())
    {
        // Paths file.
        json::Object pathsJSON = json::new_object(json_object_new_object);
        // Loop through map and write stuff.
        for (auto &[applicationID, path] : s_pathMap)
        {
            // Get ID as hex string.
            std::string idHex = stringutil::get_formatted_string("%016llX", applicationID);
            // path
            json_object *pathObject = json_object_new_string(path.c_str());

            // Add to pathsJSON object.
            json::add_object(pathsJSON, idHex.c_str(), pathObject);
        }
        // Write it.
        fslib::File pathsFile(PATH_PATHS_PATH,
                              FsOpenMode_Create | FsOpenMode_Write,
                              std::strlen(json_object_get_string(pathsJSON.get())));
        if (pathsFile)
        {
            pathsFile << json_object_get_string(pathsJSON.get());
        }
    }
}

uint8_t config::get_by_key(std::string_view key)
{
    // See if the key can be found.
    auto findKey = s_configMap.find(key.data());
    if (findKey == s_configMap.end())
    {
        return 0;
    }
    return findKey->second;
}

void config::toggle_by_key(std::string_view key)
{
    auto findKey = s_configMap.find(key.data());
    if (findKey == s_configMap.end())
    {
        return;
    }
    findKey->second = findKey->second ? 0 : 1;
}

void config::set_by_key(std::string_view key, uint8_t value)
{
    auto findKey = s_configMap.find(key.data());
    if (findKey == s_configMap.end())
    {
        return;
    }
    findKey->second = value;
}

fslib::Path config::get_working_directory()
{
    return s_workingDirectory;
}

double config::get_animation_scaling()
{
    return s_uiAnimationScaling;
}

void config::set_animation_scaling(double newScale)
{
    s_uiAnimationScaling = newScale;
}

void config::add_remove_favorite(uint64_t applicationID)
{
    auto findTitle = std::find(s_favorites.begin(), s_favorites.end(), applicationID);
    if (findTitle == s_favorites.end())
    {
        s_favorites.push_back(applicationID);
    }
    else
    {
        s_favorites.erase(findTitle);
    }
}

bool config::is_favorite(uint64_t applicationID)
{
    if (std::find(s_favorites.begin(), s_favorites.end(), applicationID) == s_favorites.end())
    {
        return false;
    }

    return true;
}

void config::add_remove_blacklist(uint64_t applicationID)
{
    auto findTitle = std::find(s_blacklist.begin(), s_blacklist.end(), applicationID);
    if (findTitle == s_blacklist.end())
    {
        s_blacklist.push_back(applicationID);
    }
    else
    {
        s_blacklist.erase(findTitle);
    }
}

bool config::is_blacklisted(uint64_t applicationID)
{
    if (std::find(s_blacklist.begin(), s_blacklist.end(), applicationID) == s_blacklist.end())
    {
        return false;
    }

    return true;
}

void config::add_custom_path(uint64_t applicationID, std::string_view customPath)
{
    s_pathMap[applicationID] = customPath.data();
}

bool config::has_custom_path(uint64_t applicationID)
{
    if (s_pathMap.find(applicationID) == s_pathMap.end())
    {
        return false;
    }

    return true;
}

void config::get_custom_path(uint64_t applicationID, char *pathOut, size_t pathOutSize)
{
    if (s_pathMap.find(applicationID) == s_pathMap.end())
    {
        return;
    }

    std::memcpy(pathOut, s_pathMap[applicationID].c_str(), s_pathMap[applicationID].length());
}

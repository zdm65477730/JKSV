#include "config.hpp"
#include "JSON.hpp"
#include "logger.hpp"
#include "stringUtil.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace
{
    // Config path(s)
    const char *CONFIG_FOLDER = "sdmc:/config/JKSV";
    const char *CONFIG_PATH = "sdmc:/config/JKSV/JKSV.json";
    // Vector to preserve order now.
    std::vector<std::pair<std::string, uint8_t>> s_configVector;
    // Working directory
    fslib::Path s_workingDirectory;
    // UI animation scaling.
    double s_uiAnimationScaling;
    // Vector of favorite title ids
    std::vector<uint64_t> s_favorites;
    // Vector of titles to ignore.
    std::vector<uint64_t> s_blacklist;
} // namespace

static void readArrayToVector(std::vector<uint64_t> &vector, json_object *array)
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

void config::initialize(void)
{
    if (!fslib::directoryExists(CONFIG_FOLDER) && !fslib::createDirectoriesRecursively(CONFIG_FOLDER))
    {
        logger::log("Error creating config folder: %s.", fslib::getErrorString());
        config::resetToDefault();
        return;
    }

    json::Object configJSON = json::newObject(json_object_from_file, CONFIG_PATH);
    if (!configJSON)
    {
        logger::log("Error opening config for reading: %s", fslib::getErrorString());
        config::resetToDefault();
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
            readArrayToVector(s_favorites, configValue);
        }
        else if (std::strcmp(keyName, config::keys::BLACKLIST.data()) == 0)
        {
            readArrayToVector(s_blacklist, configValue);
        }
        else
        {
            s_configVector.push_back(std::make_pair(keyName, json_object_get_uint64(configValue)));
        }
        json_object_iter_next(&configIterator);
    }
}

void config::resetToDefault(void)
{
    s_workingDirectory = "sdmc:/JKSV";
    s_configVector.push_back(std::make_pair(config::keys::INCLUDE_DEVICE_SAVES.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::AUTO_BACKUP_ON_RESTORE.data(), 1));
    s_configVector.push_back(std::make_pair(config::keys::AUTO_NAME_BACKUPS.data(), 1));
    s_configVector.push_back(std::make_pair(config::keys::AUTO_UPLOAD.data(), 1));
    s_configVector.push_back(std::make_pair(config::keys::HOLD_FOR_DELETION.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::HOLD_FOR_RESTORATION.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::HOLD_FOR_OVERWRITE.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::ONLY_LIST_MOUNTABLE.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::LIST_ACCOUNT_SYS_SAVES.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::ALLOW_WRITING_TO_SYSTEM.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::EXPORT_TO_ZIP.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::ZIP_COMPRESSION_LEVEL.data(), 6));
    s_configVector.push_back(std::make_pair(config::keys::TITLE_SORT_TYPE.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::JKSM_TEXT_MODE.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::FORCE_ENGLISH.data(), 0));
    s_configVector.push_back(std::make_pair(config::keys::ENABLE_TRASH_BIN.data(), 0));
    s_uiAnimationScaling = 2.5f;
}

void config::save(void)
{
    json::Object configJSON = json::newObject(json_object_new_object);

    // Add working directory first.
    json_object *workingDirectory = json_object_new_string(s_workingDirectory.cString());
    json_object_object_add(configJSON.get(), config::keys::WORKING_DIRECTORY.data(), workingDirectory);

    // Loop through map and add it.
    for (auto &[key, value] : s_configVector)
    {
        json_object *jsonValue = json_object_new_uint64(value);
        json_object_object_add(configJSON.get(), key.c_str(), jsonValue);
    }

    // Add UI scaling.
    json_object *scaling = json_object_new_double(s_uiAnimationScaling);
    json_object_object_add(configJSON.get(), config::keys::UI_ANIMATION_SCALE.data(), scaling);

    // Favorites
    json_object *favoritesArray = json_object_new_array();
    for (uint64_t &titleID : s_favorites)
    {
        // Need to do it like this or json-c does decimal instead of hex.
        json_object *newFavorite = json_object_new_string(stringutil::getFormattedString("%016lX", titleID).c_str());
        json_object_array_add(favoritesArray, newFavorite);
    }
    json_object_object_add(configJSON.get(), config::keys::FAVORITES.data(), favoritesArray);

    // Same but blacklist
    json_object *blacklistArray = json_object_new_array();
    for (uint64_t &titleID : s_blacklist)
    {
        json_object *newBlacklist = json_object_new_string(stringutil::getFormattedString("%016lX", titleID).c_str());
        json_object_array_add(blacklistArray, newBlacklist);
    }
    json_object_object_add(configJSON.get(), config::keys::BLACKLIST.data(), blacklistArray);

    // Write config file
    fslib::File configFile(CONFIG_PATH, FsOpenMode_Create | FsOpenMode_Write, std::strlen(json_object_get_string(configJSON.get())));
    configFile << json_object_get_string(configJSON.get());
}

uint8_t config::getByKey(std::string_view key)
{
    auto findKey =
        std::find_if(s_configVector.begin(), s_configVector.end(), [key](const auto &configPair) { return key == configPair.first; });
    if (findKey == s_configVector.end())
    {
        return 0;
    }
    return findKey->second;
}

uint8_t config::getByIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(s_configVector.size()))
    {
        return 0;
    }
    return s_configVector.at(index).second;
}

fslib::Path config::getWorkingDirectory(void)
{
    return s_workingDirectory;
}

double config::getAnimationScaling(void)
{
    return s_uiAnimationScaling;
}

void config::addRemoveFavorite(uint64_t applicationID)
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

bool config::isFavorite(uint64_t applicationID)
{
    if (std::find(s_favorites.begin(), s_favorites.end(), applicationID) == s_favorites.end())
    {
        return false;
    }
    return true;
}

void config::addRemoveBlacklist(uint64_t applicationID)
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

bool config::isBlacklisted(uint64_t applicationID)
{
    if (std::find(s_blacklist.begin(), s_blacklist.end(), applicationID) == s_blacklist.end())
    {
        return false;
    }
    return true;
}

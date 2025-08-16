#include "config/ConfigContext.hpp"

#include "JSON.hpp"
#include "config/keys.hpp"
#include "logging/error.hpp"
#include "stringutil.hpp"

#include <cstring>
namespace
{
    constexpr std::string_view PATH_DEFAULT_WORK_DIR = "sdmc:/JKSV";
    constexpr const char *PATH_PATHS_PATH            = "sdmc:/config/JKSV/Paths.json";
    constexpr const char *PATH_CONFIG_FILE           = "sdmc:/config/JKSV/JKSV.json";

    constexpr const char *STRING_APP_ID_FORMAT = "%016llX";
}

void config::ConfigContext::reset()
{
    m_workingDirectory                                        = PATH_DEFAULT_WORK_DIR;
    m_configMap[config::keys::INCLUDE_DEVICE_SAVES.data()]    = 0;
    m_configMap[config::keys::AUTO_BACKUP_ON_RESTORE.data()]  = 1;
    m_configMap[config::keys::AUTO_NAME_BACKUPS.data()]       = 0;
    m_configMap[config::keys::AUTO_UPLOAD.data()]             = 0;
    m_configMap[config::keys::USE_TITLE_IDS.data()]           = 0;
    m_configMap[config::keys::HOLD_FOR_DELETION.data()]       = 1;
    m_configMap[config::keys::HOLD_FOR_RESTORATION.data()]    = 1;
    m_configMap[config::keys::HOLD_FOR_OVERWRITE.data()]      = 1;
    m_configMap[config::keys::ONLY_LIST_MOUNTABLE.data()]     = 1;
    m_configMap[config::keys::LIST_ACCOUNT_SYS_SAVES.data()]  = 0;
    m_configMap[config::keys::ALLOW_WRITING_TO_SYSTEM.data()] = 0;
    m_configMap[config::keys::EXPORT_TO_ZIP.data()]           = 1;
    m_configMap[config::keys::ZIP_COMPRESSION_LEVEL.data()]   = 6;
    m_configMap[config::keys::TITLE_SORT_TYPE.data()]         = 0;
    m_configMap[config::keys::JKSM_TEXT_MODE.data()]          = 0;
    m_configMap[config::keys::FORCE_ENGLISH.data()]           = 0;
    m_configMap[config::keys::ENABLE_TRASH_BIN.data()]        = 0;
    m_animationScaling                                        = 2.5f;
}

void config::ConfigContext::save()
{

    json::Object configJSON = json::new_object(json_object_new_object);
    if (!configJSON) { return; }

    json_object *workDir = json_object_new_string(m_workingDirectory.full_path());
    json::add_object(configJSON, config::keys::WORKING_DIRECTORY.data(), workDir);

    for (const auto &[key, value] : m_configMap)
    {
        json_object *valueObject = json_object_new_uint64(value);
        json::add_object(configJSON, key.c_str(), valueObject);
    }

    json_object *scaling = json_object_new_double(m_animationScaling);
    json::add_object(configJSON, config::keys::UI_ANIMATION_SCALE.data(), scaling);

    json_object *favoritesArray = json_object_new_array();
    for (const uint64_t &applicationID : m_favorites)
    {
        const std::string appIDHex = stringutil::get_formatted_string(STRING_APP_ID_FORMAT, applicationID);
        json_object *newFavorite   = json_object_new_string(appIDHex.c_str());

        json_object_array_add(favoritesArray, newFavorite);
    }
    json::add_object(configJSON, config::keys::FAVORITES.data(), favoritesArray);

    json_object *blacklistArray = json_object_new_array();
    for (const uint64_t &applicationID : m_blacklist)
    {
        const std::string appIDHex = stringutil::get_formatted_string(STRING_APP_ID_FORMAT, applicationID);
        json_object *newBlacklist  = json_object_new_string(appIDHex.c_str());

        json_object_array_add(blacklistArray, newBlacklist);
    }
    json::add_object(configJSON, config::keys::BLACKLIST.data(), blacklistArray);

    const char *configString   = json::get_string(configJSON);
    const int64_t configLength = std::char_traits<char>::length(configString);
    fslib::File configFile{PATH_CONFIG_FILE, FsOpenMode_Create | FsOpenMode_Write, configLength};
    if (configFile.is_open()) { configFile << configString; }
}

void config::ConfigContext::load()
{
    json::Object configJSON = json::new_object(json_object_from_file, PATH_CONFIG_FILE);
    if (!configJSON)
    {
        error::fslib(false); // This seems weird, but it should catch the problem.
        return;
    }

    json_object_iterator configIter = json::iter_begin(configJSON);
    json_object_iterator configEnd  = json::iter_end(configJSON);
    while (!json_object_iter_equal(&configIter, &configEnd))
    {
        const char *key    = json_object_iter_peek_name(&configIter);
        json_object *value = json_object_iter_peek_value(&configIter);

        const bool workingDir = std::strcmp(config::keys::WORKING_DIRECTORY.data(), key) == 0;
        const bool scaling    = std::strcmp(config::keys::UI_ANIMATION_SCALE.data(), key) == 0;
        const bool favorites  = std::strcmp(config::keys::FAVORITES.data(), key) == 0;
        const bool blacklist  = std::strcmp(config::keys::BLACKLIST.data(), key) == 0;

        if (workingDir) { m_workingDirectory = json_object_get_string(value); }
        else if (scaling) { m_animationScaling = json_object_get_double(value); }
        else if (favorites) { ConfigContext::read_array_to_vector(m_favorites, value); }
        else if (blacklist) { ConfigContext::read_array_to_vector(m_blacklist, value); }
        else { m_configMap[key] = json_object_get_uint64(value); }

        json_object_iter_next(&configIter);
    }

    const fslib::Path pathsPath{PATH_PATHS_PATH};
    const bool pathsExists = fslib::file_exists(pathsPath);
    if (!pathsExists) { return; }

    json::Object pathsJSON = json::new_object(json_object_from_file, pathsPath.full_path());
    if (!pathsJSON)
    {
        error::fslib(false);
        return;
    }

    json_object_iterator pathsIter = json::iter_begin(configJSON);
    json_object_iterator pathsEnd  = json::iter_end(configJSON);
    while (!json_object_iter_equal(&pathsIter, &pathsEnd))
    {
        const char *appIDString = json_object_iter_peek_name(&pathsIter);
        json_object *pathObject = json_object_iter_peek_value(&pathsIter);

        const uint64_t applicationID = std::strtoull(appIDString, nullptr, 16);
        const char *path             = json_object_get_string(pathObject);
        m_pathMap[applicationID]     = path;

        json_object_iter_next(&pathsIter);
    }
}

uint8_t config::ConfigContext::get_by_key(std::string_view key)
{
    auto findKey = m_configMap.find(key.data());
    if (findKey == m_configMap.end()) { return 0; }
    return findKey->second;
}

void config::ConfigContext::toggle_by_key(std::string_view key)
{
    auto findKey = m_configMap.find(key.data());
    if (findKey == m_configMap.end()) { return; }
    findKey->second = findKey->second ? 0 : 1;
}

void config::ConfigContext::set_by_key(std::string_view key, uint8_t value)
{
    auto findKey = m_configMap.find(key.data());
    if (findKey == m_configMap.end()) { return; }
    findKey->second = value;
}

fslib::Path config::ConfigContext::get_working_directory() const { return m_workingDirectory; }

bool config::ConfigContext::set_working_directory(std::string_view path)
{
    fslib::Path testPath{path};
    if (!testPath.is_valid()) { return false; }

    m_workingDirectory = std::move(testPath);
    return true;
}

double config::ConfigContext::get_animation_scaling() const { return m_animationScaling; }

void config::ConfigContext::set_animation_scaling(double scaling) { m_animationScaling = scaling; }

void config::ConfigContext::add_favorite(uint64_t applicationID)
{
    auto findID = ConfigContext::find_favorite(applicationID);
    if (findID != m_favorites.end()) { return; }
    m_favorites.push_back(applicationID);
}

void config::ConfigContext::remove_favorite(uint64_t applicationID)
{
    auto findID = ConfigContext::find_favorite(applicationID);
    if (findID == m_favorites.end()) { return; }
    m_favorites.erase(findID);
}

bool config::ConfigContext::is_favorite(uint64_t applicationID)
{
    return ConfigContext::find_favorite(applicationID) != m_favorites.end();
}

void config::ConfigContext::add_to_blacklist(uint64_t applicationID)
{
    auto findID = ConfigContext::find_blacklist(applicationID);
    if (findID != m_blacklist.end()) { return; }
    m_blacklist.push_back(applicationID);
}

void config::ConfigContext::remove_from_blacklist(uint64_t applicationID)
{
    auto findID = ConfigContext::find_blacklist(applicationID);
    if (findID == m_blacklist.end()) { return; }
    m_blacklist.erase(findID);
}

void config::ConfigContext::get_blacklisted_titles(std::vector<uint64_t> &listOut)
{
    listOut.clear();
    listOut.assign(m_blacklist.begin(), m_blacklist.end());
}

bool config::ConfigContext::is_blacklisted(uint64_t applicationID)
{
    return ConfigContext::find_blacklist(applicationID) != m_blacklist.end();
}

bool config::ConfigContext::blacklist_is_empty() const { return m_blacklist.empty(); }

void config::ConfigContext::add_custom_path(uint64_t applicationID, std::string_view path)
{
    m_pathMap[applicationID] = path.data();
    ConfigContext::save_custom_paths();
}

bool config::ConfigContext::has_custom_path(uint64_t applicationID) { return m_pathMap.find(applicationID) != m_pathMap.end(); }

void config::ConfigContext::get_custom_path(uint64_t applicationID, char *pathBuffer, size_t bufferSize)
{
    if (!ConfigContext::has_custom_path(applicationID)) { return; }

    const std::string &path = m_pathMap[applicationID];
    if (path.length() > bufferSize) { return; }

    std::memset(pathBuffer, 0x00, bufferSize);
    std::memcpy(pathBuffer, path.c_str(), path.length());
}

void config::ConfigContext::read_array_to_vector(std::vector<uint64_t> &vector, json_object *array)
{
    const int arrayLength = json_object_array_length(array);
    for (int i = 0; i < arrayLength; i++)
    {
        json_object *arrayElement = json_object_array_get_idx(array, i);
        if (!arrayElement) { break; }

        const char *appIDStr         = json_object_get_string(arrayElement);
        const uint64_t applicationID = std::strtoull(appIDStr, nullptr, 16);
        vector.push_back(applicationID);
    }
}

void config::ConfigContext::save_custom_paths()
{
    json::Object pathsJSON = json::new_object(json_object_new_object);
    for (auto &[applicationID, outputPath] : m_pathMap)
    {
        const std::string appIDHex = stringutil::get_formatted_string(STRING_APP_ID_FORMAT, applicationID);
        const char *path           = outputPath.c_str();

        json_object *outputString = json_object_new_string(path);
        json::add_object(pathsJSON, appIDHex, outputString);
    }

    const char *pathsString   = json::get_string(pathsJSON);
    const int64_t pathsLength = std::char_traits<char>::length(pathsString);
    fslib::File pathsFile{PATH_PATHS_PATH, FsOpenMode_Create | FsOpenMode_Write, pathsLength};
    if (pathsFile.is_open()) { pathsFile << pathsString; };
}

config::ConfigContext::AppIDList::iterator config::ConfigContext::find_favorite(uint64_t applicationID)
{
    return std::find(m_favorites.begin(), m_favorites.end(), applicationID);
}

config::ConfigContext::AppIDList::iterator config::ConfigContext::find_blacklist(uint64_t applicationID)
{
    return std::find(m_blacklist.begin(), m_blacklist.end(), applicationID);
}

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace cfg
{
    typedef enum
    {
        ALPHA,
        MOST_PLAYED,
        LAST_PLAYED
    } sortTypes;

    void resetConfig();
    void loadConfig();
    void saveConfig();

    bool isBlacklisted(uint64_t tid);
    void addTitleToBlacklist(void *a);
    void removeTitleFromBlacklist(uint64_t tid);

    bool isFavorite(uint64_t tid);
    void addTitleToFavorites(uint64_t tid);

    bool isDefined(uint64_t tid);
    void pathDefAdd(uint64_t tid, const std::string &newPath);
    std::string getPathDefinition(uint64_t tid);

    void addPathToFilter(uint64_t tid, const std::string &_p);

    extern std::unordered_map<std::string, bool> config;
    extern std::vector<uint64_t> blacklist;
    extern std::vector<uint64_t> favorites;
    extern uint8_t sortType;
    extern std::string driveClientID, driveClientSecret, driveRefreshToken;
    extern std::string webdavOrigin, webdavBasePath, webdavUser, webdavPassword;
} // namespace cfg

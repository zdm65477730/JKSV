#include "data/data.hpp"

#include "config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "logger.hpp"
#include "strings.hpp"

#include <algorithm>
#include <array>
#include <string_view>
#include <switch.h>
#include <unordered_map>
#include <vector>

namespace
{
    // This is easer to read imo
    using UserIDPair = std::pair<AccountUid, data::User>;

    /// @brief Struct used for reading the cache from file.
    // clang-format off
    typedef struct
    {
        uint64_t applicationID;
        NsApplicationControlData data;
    } CacheEntry;
    // clang-format on

    // User vector to preserve order.
    std::vector<UserIDPair> s_userVector;

    // Map of Title info paired with its title/application
    std::unordered_map<uint64_t, data::TitleInfo> s_titleInfoMap;

    /// @brief Path used for cacheing title information since NS got slow on 20.0+
    constexpr std::string_view PATH_CACHE_PATH = "sdmc:/config/JKSV/cache.bin";

    // These are the ID's used for system type users.
    constexpr AccountUid ID_SYSTEM_USER = {FsSaveDataType_System};
    constexpr AccountUid ID_BCAT_USER   = {FsSaveDataType_Bcat};
    constexpr AccountUid ID_DEVICE_USER = {FsSaveDataType_Device};
    constexpr AccountUid ID_CACHE_USER  = {FsSaveDataType_Cache};
} // namespace

// Declarations here. Definitions at bottom. These should appear in the order called.
/// @brief Loads users from the system and creates the system users.
static bool load_create_user_accounts();

/// @brief Loads the application records from NS.
static void load_application_records();

/// @brief Imports external SVI(Control Data) files.
static void import_svi_files();

/// @brief Attempts to read the cache file from the SD.
/// @return True on success. False on failure.
static bool read_cache_file();

/// @brief Creates the cache file on the SD card.
static void create_cache_file();

bool data::initialize(bool clearCache)
{
    const fslib::Path cachePath{PATH_CACHE_PATH};
    const bool cacheExists      = fslib::file_exists(cachePath);
    const bool cacheClearFailed = clearCache && cacheExists && error::fslib(fslib::delete_file(cachePath));
    if (clearCache && cacheClearFailed) { return false; }

    const bool needsUsers  = s_userVector.empty();
    const bool usersLoaded = needsUsers && load_create_user_accounts();
    if (needsUsers && !usersLoaded) { return false; }

    const bool cacheRead = read_cache_file();
    if (!cacheRead)
    {
        s_titleInfoMap.clear();
        load_application_records();
        import_svi_files();
    }

    for (auto &[accountID, user] : s_userVector) { user.load_user_data(); }

    const bool needsCacheBuild = !fslib::file_exists(cachePath);
    if (needsCacheBuild) { create_cache_file(); }

    return true;
}

void data::get_users(data::UserList &userList)
{
    userList.clear();
    for (auto &[accountID, userData] : s_userVector) { userList.push_back(&userData); }
}

data::TitleInfo *data::get_title_info_by_id(uint64_t applicationID)
{
    auto findTitle = s_titleInfoMap.find(applicationID);
    if (findTitle == s_titleInfoMap.end()) { return nullptr; }
    return &findTitle->second;
}

void data::load_title_to_map(uint64_t applicationID) { s_titleInfoMap.emplace(applicationID, applicationID); }

bool data::title_exists_in_map(uint64_t applicationID) { return s_titleInfoMap.find(applicationID) != s_titleInfoMap.end(); }

std::unordered_map<uint64_t, data::TitleInfo> &data::get_title_info_map() { return s_titleInfoMap; }

void data::get_title_info_by_type(FsSaveDataType saveType, std::vector<data::TitleInfo *> &vectorOut)
{
    // Clear vector JIC
    vectorOut.clear();

    // Loop and push pointers
    for (auto &[applicationID, titleInfo] : s_titleInfoMap)
    {
        if (titleInfo.has_save_data_type(saveType)) { vectorOut.push_back(&titleInfo); }
    }
}

static bool load_create_user_accounts()
{
    const char *systemName = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 0);
    const char *bcatName   = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 2);
    const char *deviceName = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 3);
    const char *cacheName  = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 5);

    int total{};
    AccountUid accounts[8]{};
    const bool accountError = error::libnx(accountListAllUsers(accounts, 8, &total));
    const bool noAccounts   = total <= 0;
    if (accountError || noAccounts) { return false; }

    // Loop and load the account data.
    for (int i = 0; i < total; i++)
    {
        auto userPair = std::make_pair(accounts[i], std::move(data::User(accounts[i], FsSaveDataType_Account)));
        s_userVector.push_back(std::move(userPair));
    }

    // Create and push the system type users. Path safe names are hard coded to English for these.
    auto devicePair = std::make_pair(ID_DEVICE_USER, data::User(ID_DEVICE_USER, deviceName, "Device", FsSaveDataType_Device));
    auto bcatPair   = std::make_pair(ID_BCAT_USER, data::User(ID_BCAT_USER, bcatName, "BCAT", FsSaveDataType_Bcat));
    auto cachePair  = std::make_pair(ID_CACHE_USER, data::User(ID_CACHE_USER, cacheName, "Cache", FsSaveDataType_Cache));
    auto systemPair = std::make_pair(ID_SYSTEM_USER, data::User(ID_SYSTEM_USER, systemName, "System", FsSaveDataType_System));
    s_userVector.push_back(std::move(devicePair));
    s_userVector.push_back(std::move(bcatPair));
    s_userVector.push_back(std::move(cachePair));
    s_userVector.push_back(std::move(systemPair));

    return true;
}

static void load_application_records()
{
    int offset{};
    int count{};
    NsApplicationRecord record{};

    bool listError{};
    do {
        listError = error::libnx(nsListApplicationRecord(&record, 1, offset++, &count)) || count <= 0;
        if (listError) { break; }
        s_titleInfoMap.emplace(record.application_id, record.application_id);
    } while (!listError);
}

static void import_svi_files()
{
    static constexpr size_t SIZE_UINT64    = sizeof(uint64_t);
    static constexpr size_t SIZE_CTRL_DATA = sizeof(NsApplicationControlData);
    static constexpr size_t SIZE_SVI       = SIZE_UINT64 + SIZE_CTRL_DATA;

    const fslib::Path sviPath = config::get_working_directory() / "svi";
    const fslib::Directory sviDir{sviPath};
    if (error::fslib(sviDir)) { return; }

    const int64_t sviCount = sviDir.get_count();
    for (int64_t i = 0; i < sviCount; i++)
    {
        const fslib::Path target = sviPath / sviDir[i];
        fslib::File sviFile{target, FsOpenMode_Read};

        const bool goodFile = sviFile.is_open() && sviFile.get_size() == SIZE_SVI;
        if (!goodFile) { continue; }

        uint64_t applicationID{};
        NsApplicationControlData controlData{};
        const bool idReadGood   = sviFile.read(&applicationID, SIZE_UINT64) == SIZE_UINT64;
        const bool dataReadGood = sviFile.read(&controlData, SIZE_CTRL_DATA) == SIZE_CTRL_DATA;
        if (!idReadGood || !dataReadGood) { continue; }

        data::TitleInfo newInfo{applicationID, controlData};
        s_titleInfoMap.emplace(applicationID, std::move(newInfo));
    }
}

static bool read_cache_file()
{
    constexpr size_t SIZE_UNSIGNED    = sizeof(unsigned int);
    constexpr size_t SIZE_CACHE_ENTRY = sizeof(CacheEntry);

    const fslib::Path cachePath{PATH_CACHE_PATH};
    const bool cacheExists = fslib::file_exists(cachePath);

    fslib::File cache{PATH_CACHE_PATH, FsOpenMode_Read};
    if (error::fslib(cache.is_open())) { return false; }

    unsigned int titleCount{};
    const bool countRead = cache.read(&titleCount, SIZE_UNSIGNED) == SIZE_UNSIGNED;
    if (!countRead) { return false; }

    auto entryBuffer = std::make_unique<CacheEntry[]>(titleCount); // I've read there might not be any point in error checking.
    const bool cacheRead = cache.read(entryBuffer.get(), SIZE_CACHE_ENTRY * titleCount) == SIZE_CACHE_ENTRY * titleCount;
    if (!cacheRead) { return false; }

    // Loop through the cache entries and emplace them to the map.
    for (unsigned int i = 0; i < titleCount; i++)
    {
        CacheEntry &entry = entryBuffer[i];

        data::TitleInfo newInfo{entry.applicationID, entry.data};
        s_titleInfoMap.emplace(entry.applicationID, std::move(newInfo));
    }
    return true;
}

static void create_cache_file()
{
    static constexpr size_t SIZE_UNSIGNED  = sizeof(unsigned int);
    static constexpr size_t SIZE_UINT64    = sizeof(uint64_t);
    static constexpr size_t SIZE_CTRL_DATA = sizeof(NsApplicationControlData);

    fslib::File cache{PATH_CACHE_PATH, FsOpenMode_Create | FsOpenMode_Write};
    if (error::fslib(cache.is_open())) { return; }

    // This will make more sense later. I promise.
    unsigned int titleCount{};
    const bool countWrite = cache.write(&titleCount, SIZE_UNSIGNED) == SIZE_UNSIGNED;
    if (!countWrite) { return; }

    for (auto &[applicationID, titleInfo] : s_titleInfoMap)
    {
        if (!titleInfo.has_control_data()) { continue; }

        const NsApplicationControlData *data = titleInfo.get_control_data();
        const bool idWrite                   = cache.write(&applicationID, SIZE_UINT64) == SIZE_UINT64;
        const bool dataWrite                 = cache.write(data, SIZE_CTRL_DATA) == SIZE_CTRL_DATA;
        if (!idWrite || !dataWrite) { return; }
        ++titleCount;
    }

    // Go back to beginning and write the final count.
    cache.seek(0, cache.BEGINNING);
    cache.write(&titleCount, SIZE_UNSIGNED);
}

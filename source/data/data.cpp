#include "data/data.hpp"

#include "config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include "stringutil.hpp"

#include <algorithm>
#include <array>
#include <mutex>
#include <string>
#include <switch.h>
#include <thread>
#include <unordered_map>
#include <vector>

namespace
{
    /// @brief This contains the user accounts on the system.
    std::vector<data::User> s_users{};

    // Map of Title info paired with its title/application
    std::unordered_map<uint64_t, data::TitleInfo> s_titleinfo;

    /// @brief Path used for cacheing title information since NS got slow on 20.0+
    constexpr std::string_view PATH_CACHE_PATH = "sdmc:/config/JKSV/cache.zip";

    // These are the ID's used for system type users.
    constexpr AccountUid ID_SYSTEM_USER = {FsSaveDataType_System};
    constexpr AccountUid ID_BCAT_USER   = {FsSaveDataType_Bcat};
    constexpr AccountUid ID_DEVICE_USER = {FsSaveDataType_Device};
    constexpr AccountUid ID_CACHE_USER  = {FsSaveDataType_Cache};

    /// @brief This is for loading the cache.
    constexpr size_t SIZE_CTRL_DATA = sizeof(NsApplicationControlData);
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

    const bool usersLoaded = s_users.empty() && load_create_user_accounts();
    if (!usersLoaded && s_users.empty()) { return false; }

    if (!read_cache_file())
    {
        s_titleinfo.clear();
        load_application_records();
        import_svi_files();
    }

    for (data::User &user : s_users) { user.load_user_data(); }

    const bool needsCache = !fslib::file_exists(cachePath);
    if (needsCache) { create_cache_file(); }

    return true;
}

void data::get_users(data::UserList &userList)
{
    for (data::User &user : s_users) { userList.push_back(&user); }
}

data::TitleInfo *data::get_title_info_by_id(uint64_t applicationID)
{
    auto findTitle = s_titleinfo.find(applicationID);
    if (findTitle == s_titleinfo.end()) { return nullptr; }
    return &findTitle->second;
}

void data::load_title_to_map(uint64_t applicationID) { s_titleinfo.emplace(applicationID, applicationID); }

bool data::title_exists_in_map(uint64_t applicationID) { return s_titleinfo.find(applicationID) != s_titleinfo.end(); }

void data::get_title_info_list(data::TitleInfoList &listOut)
{
    for (auto &[applicationID, titleInfo] : s_titleinfo) { listOut.push_back(&titleInfo); }
}

void data::get_title_info_by_type(FsSaveDataType saveType, data::TitleInfoList &listOut)
{
    for (auto &[applicationID, titleInfo] : s_titleinfo)
    {
        if (titleInfo.has_save_data_type(saveType)) { listOut.push_back(&titleInfo); }
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
    for (int i = 0; i < total; i++) { s_users.emplace_back(accounts[i], FsSaveDataType_Account); }
    s_users.emplace_back(ID_DEVICE_USER, deviceName, "Device", FsSaveDataType_Device);
    s_users.emplace_back(ID_BCAT_USER, bcatName, "BCAT", FsSaveDataType_Bcat);
    s_users.emplace_back(ID_CACHE_USER, cacheName, "Cache", FsSaveDataType_Cache);
    s_users.emplace_back(ID_SYSTEM_USER, systemName, "System", FsSaveDataType_System);

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
        s_titleinfo.emplace(record.application_id, record.application_id);
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
        s_titleinfo.emplace(applicationID, std::move(newInfo));
    }
}

static bool read_cache_file()
{
    fs::MiniUnzip cacheZip{PATH_CACHE_PATH};
    if (!cacheZip.is_open()) { return false; }

    NsApplicationControlData controlBuffer{};
    do {
        const bool read = cacheZip.read(&controlBuffer, SIZE_CTRL_DATA) == SIZE_CTRL_DATA;
        if (!read) { continue; }

        const char *filename         = cacheZip.get_filename();
        const uint64_t applicationID = std::strtoull(filename, nullptr, 16);
        data::TitleInfo newInfo{applicationID, controlBuffer};

        s_titleinfo.emplace(applicationID, std::move(newInfo));
    } while (cacheZip.next_file());

    return true;
}

static void create_cache_file()
{
    fs::MiniZip cacheZip{PATH_CACHE_PATH};
    if (!cacheZip.is_open()) { return; }

    for (auto &[applicationID, titleInfo] : s_titleinfo)
    {
        if (!titleInfo.has_control_data()) { continue; }

        const NsApplicationControlData *controlData = titleInfo.get_control_data();
        const std::string cacheName                 = stringutil::get_formatted_string("%016llX", applicationID);
        const bool opened                           = cacheZip.open_new_file(cacheName);
        if (!opened) { continue; }

        const bool controlWritten = cacheZip.write(controlData, SIZE_CTRL_DATA);
        if (!controlWritten) { logger::log("Error writing control data to zip!"); }
        cacheZip.close_current_file();
    }
}

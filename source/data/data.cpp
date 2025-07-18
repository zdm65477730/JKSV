#include "data/data.hpp"
#include "config.hpp"
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
    typedef struct
    {
            uint64_t m_applicationID;
            NsApplicationControlData m_data;
    } CacheEntry;

    // User vector to preserve order.
    std::vector<UserIDPair> s_userVector;

    // Map of Title info paired with its title/application
    std::unordered_map<uint64_t, data::TitleInfo> s_titleInfoMap;

    /// @brief Path used for cacheing title information since NS got slow on 20.0+
    constexpr std::string_view PATH_CACHE_PATH = "sdmc:/config/JKSV/cache.bin";

    // These are the ID's used for system type users.
    constexpr AccountUid ID_SYSTEM_USER = {FsSaveDataType_System};
    constexpr AccountUid ID_BCAT_USER = {FsSaveDataType_Bcat};
    constexpr AccountUid ID_DEVICE_USER = {FsSaveDataType_Device};
    constexpr AccountUid ID_CACHE_USER = {FsSaveDataType_Cache};
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
    // Convert this to an fslib::Path right off the bat so we don't call the path constructor twice.
    fslib::Path cachePath = PATH_CACHE_PATH;

    // Nuke the cache file if we're supposed to.
    if (clearCache && fslib::file_exists(cachePath) && !fslib::delete_file(cachePath))
    {
        // I don't really think this should be fatal. It's not good that it happens, but not fatal.
        logger::log("data::initialize failed to remove existing cache file from SD: %s", fslib::error::get_string());
    }

    // Load user accounts if not done previously. Bail if the load fails cause that is fatal.
    if (s_userVector.empty() && !load_create_user_accounts())
    {
        return false;
    }

    // If the cacheWas nuked, the map is empty or we fail to read the cache file...
    if ((clearCache || s_titleInfoMap.empty()) && !read_cache_file())
    {
        // Clear the map out first.
        s_titleInfoMap.clear();
        // Load the application records.
        load_application_records();
    }

    // Load these now if needed.
    import_svi_files();

    // Load the save data.
    // data::load_save_data_info();
    // Loop users and make them load their data.
    for (auto &[accountID, user] : s_userVector)
    {
        user.load_user_data();
    }


    // If the cache file doesn't exist at this point, create it.
    if (!fslib::file_exists(PATH_CACHE_PATH))
    {
        create_cache_file();
    }

    // Wew
    return true;
}

void data::get_users(data::UserList &userList)
{
    // Clear the vector passed just in case.
    userList.clear();

    // Loop and push the pointers to the vector.
    for (auto &[accountID, userData] : s_userVector)
    {
        userList.push_back(&userData);
    }
}

data::TitleInfo *data::get_title_info_by_id(uint64_t applicationID)
{
    if (s_titleInfoMap.find(applicationID) == s_titleInfoMap.end())
    {
        return nullptr;
    }

    return &s_titleInfoMap.at(applicationID);
}

void data::load_title_to_map(uint64_t applicationID)
{
    s_titleInfoMap.emplace(applicationID, applicationID);
}

bool data::title_exists_in_map(uint64_t applicationID)
{
    return s_titleInfoMap.find(applicationID) != s_titleInfoMap.end();
}

std::unordered_map<uint64_t, data::TitleInfo> &data::get_title_info_map()
{
    return s_titleInfoMap;
}

void data::get_title_info_by_type(FsSaveDataType saveType, std::vector<data::TitleInfo *> &vectorOut)
{
    // Clear vector JIC
    vectorOut.clear();

    // Loop and push pointers
    for (auto &[applicationID, titleInfo] : s_titleInfoMap)
    {
        if (titleInfo.has_save_data_type(saveType))
        {
            vectorOut.push_back(&titleInfo);
        }
    }
}

static bool load_create_user_accounts()
{
    // For saving total accounts found.
    int total = 0;

    // The Switch can only have up to eight user accounts.
    AccountUid accounts[8] = {0};

    // Try to list all users on the system.
    if (R_FAILED(accountListAllUsers(accounts, 8, &total)) || total <= 0)
    {
        // This is fatal.
        logger::log("Error listing user accounts!");
        return false;
    }

    // Loop and load the account data.
    for (int i = 0; i < total; i++)
    {
        s_userVector.push_back(std::make_pair(accounts[i], data::User(accounts[i], FsSaveDataType_Account)));
    }

    // Create and push the system type users. Path safe names are hard coded to English for these.
    // Clang-format makes these hard to read.
    s_userVector.push_back(std::make_pair(ID_DEVICE_USER,
                                          data::User(ID_DEVICE_USER,
                                                     strings::get_by_name(strings::names::SAVE_DATA_TYPES, 3),
                                                     "Device",
                                                     FsSaveDataType_Device)));
    s_userVector.push_back(std::make_pair(ID_BCAT_USER,
                                          data::User(ID_BCAT_USER,
                                                     strings::get_by_name(strings::names::SAVE_DATA_TYPES, 2),
                                                     "BCAT",
                                                     FsSaveDataType_Bcat)));
    s_userVector.push_back(std::make_pair(ID_CACHE_USER,
                                          data::User(ID_CACHE_USER,
                                                     strings::get_by_name(strings::names::SAVE_DATA_TYPES, 5),
                                                     "Cache",
                                                     FsSaveDataType_Cache)));
    s_userVector.push_back(std::make_pair(ID_SYSTEM_USER,
                                          data::User(ID_SYSTEM_USER,
                                                     strings::get_by_name(strings::names::SAVE_DATA_TYPES, 0),
                                                     "System",
                                                     FsSaveDataType_System)));

    // Should be good to go?
    return true;
}

static void load_application_records()
{
    // Record struct/buffer.
    NsApplicationRecord record = {0};
    // Current entry offset.
    int offset = 0;
    // Current entry count. This is done one by one.
    int count = 0;

    // Loop and read the records into the map.
    while (R_SUCCEEDED(nsListApplicationRecord(&record, 1, offset++, &count)) && count > 0)
    {
        // s_titleInfoMap.emplace(record.application_id, data::TitleInfo(record.application_id));
        s_titleInfoMap.emplace(record.application_id, record.application_id);
    }
}

static void import_svi_files()
{
    // Path.
    fslib::Path sviPath = config::get_working_directory() / "svi";

    // Try opening it.
    fslib::Directory sviDir(sviPath);
    if (!sviDir)
    {
        return;
    }

    // Loop through the directory and load these things.
    for (int64_t i = 0; i < sviDir.get_count(); i++)
    {
        // Full path.
        fslib::Path fullPath = sviPath / sviDir[i];

        // Try opening it.
        fslib::File sviFile(fullPath, FsOpenMode_Read);
        if (!sviFile || sviFile.get_size() != sizeof(uint64_t) + sizeof(NsApplicationControlData))
        {
            logger::log("Error importing \"%s\": File couldn't be opened or is invalid!");
            continue;
        }

        // First read the ID so we can check if it's even worth bothering with the rest.
        // To do. This could be accomplished with just the file name when I have time.
        uint64_t applicationID = 0;
        if (sviFile.read(&applicationID, sizeof(uint64_t)) != sizeof(uint64_t) ||
            s_titleInfoMap.find(applicationID) != s_titleInfoMap.end())
        {
            // Not worth continuing with this because it was already loaded somewhere else.
            continue;
        }

        // Good to go and read this now.
        NsApplicationControlData controlData = {0};
        if (sviFile.read(&controlData, sizeof(NsApplicationControlData)) != sizeof(NsApplicationControlData))
        {
            logger::log("Error reading SVI file!");
            continue;
        }

        s_titleInfoMap.emplace(applicationID, std::move(data::TitleInfo(applicationID, controlData)));
    }
}

static bool read_cache_file()
{
    // Try opening the cache file.
    fslib::File cache(PATH_CACHE_PATH, FsOpenMode_Read);
    if (!cache)
    {
        logger::log("No cache file found!");
        return false;
    }

    // Title cache count.
    unsigned int titleCount = 0;
    // Read it
    cache.read(&titleCount, sizeof(unsigned int));

    // Allocate buffer for reading the rest.
    std::unique_ptr<CacheEntry[]> entryBuffer = std::make_unique<CacheEntry[]>(titleCount);
    if (!entryBuffer)
    {
        logger::log("Error allocating memory to read cache!");
        return false;
    }

    if (cache.read(entryBuffer.get(), sizeof(CacheEntry) * titleCount) !=
        static_cast<ssize_t>(sizeof(CacheEntry) * titleCount))
    {
        logger::log("Error reading cache file! Size mismatch!");
        return false;
    }

    // Loop through the cache entries and emplace them to the map.
    for (unsigned int i = 0; i < titleCount; i++)
    {
        s_titleInfoMap.emplace(entryBuffer[i].m_applicationID,
                               std::move(data::TitleInfo(entryBuffer[i].m_applicationID, entryBuffer[i].m_data)));
    }

    return true;
}

static void create_cache_file()
{
    // First try creating and opening the file.
    fslib::File cache(PATH_CACHE_PATH, FsOpenMode_Create | FsOpenMode_Write);
    if (!cache)
    {
        // Don't bother trying to continue.
        return;
    }

    // This is to keep track of how many titles we actually write to the file.
    unsigned int titleCount = 0;

    // Start by writing that. We'll come back to it later.
    cache.write(&titleCount, sizeof(unsigned int));

    // Loop through the title map and write what we need to.
    for (auto &[applicationID, titleInfo] : s_titleInfoMap)
    {
        // Check if the title has valid control data before continuing.
        if (!titleInfo.has_control_data())
        {
            continue;
        }

        // Grab the control data pointer.
        NsApplicationControlData *data = titleInfo.get_control_data();

        // Write the application ID first.
        cache.write(&applicationID, sizeof(uint64_t));
        // Write it to the file.
        cache.write(data, sizeof(NsApplicationControlData));

        // Increment the counter.
        ++titleCount;
    }

    // Go back to the beginning.
    cache.seek(0, cache.BEGINNING);

    // Write the count.
    cache.write(&titleCount, sizeof(unsigned int));
    // fslib::File cleans up on destruction.
}

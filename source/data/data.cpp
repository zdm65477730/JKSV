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

    /// @brief This is the struct used for caching title info for 20.0+.
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

    // Array of SaveDataSpaceIDs - SaveDataSpaceAll doesn't seem to work as it should...
    constexpr std::array<FsSaveDataSpaceId, 7> SAVE_DATA_SPACE_ORDER = {FsSaveDataSpaceId_System,
                                                                        FsSaveDataSpaceId_User,
                                                                        FsSaveDataSpaceId_SdSystem,
                                                                        FsSaveDataSpaceId_Temporary,
                                                                        FsSaveDataSpaceId_SdUser,
                                                                        FsSaveDataSpaceId_ProperSystem,
                                                                        FsSaveDataSpaceId_SafeMode};
} // namespace

// Declarations here. Definitions at bottom. These should appear in the order called.
/// @brief Loads users from the system and creates the system users.
static bool load_create_user_accounts(void);

/// @brief Loads the application records from NS.
static void load_application_records(void);

/// @brief Loads the save data info available from the system.
static void load_save_data_info(void);

/// @brief Attempts to read the cache file from the SD.
/// @return True on success. False on failure.
static bool read_cache_file(void);

/// @brief Creates the cache file on the SD card.
static void create_cache_file(void);

bool data::initialize(void)
{
    // Load user accounts if not done previously. Bail if the load fails.
    if (s_userVector.empty() && !load_create_user_accounts())
    {
        return false;
    }

    // Attempt to read the cache file from SD. If not, load application records and create the cache.
    if (!read_cache_file())
    {
        load_application_records();
    }

    // Load the save data.
    load_save_data_info();

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

std::unordered_map<uint64_t, data::TitleInfo> &data::get_title_info_map(void)
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

static bool load_create_user_accounts(void)
{
    // These are the IDs used for system type account users.
    constexpr AccountUid deviceID = {FsSaveDataType_Device};
    constexpr AccountUid bcatID = {FsSaveDataType_Bcat};
    constexpr AccountUid cacheID = {FsSaveDataType_Cache};
    constexpr AccountUid systemID = {FsSaveDataType_System};

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
    s_userVector.push_back(std::make_pair(deviceID,
                                          data::User(deviceID,
                                                     strings::get_by_name(strings::names::SAVE_DATA_TYPES, 3),
                                                     "Device",
                                                     FsSaveDataType_Device)));
    s_userVector.push_back(std::make_pair(
        bcatID,
        data::User(bcatID, strings::get_by_name(strings::names::SAVE_DATA_TYPES, 2), "BCAT", FsSaveDataType_Bcat)));
    s_userVector.push_back(std::make_pair(
        cacheID,
        data::User(cacheID, strings::get_by_name(strings::names::SAVE_DATA_TYPES, 5), "Cache", FsSaveDataType_Cache)));
    s_userVector.push_back(std::make_pair(systemID,
                                          data::User(systemID,
                                                     strings::get_by_name(strings::names::SAVE_DATA_TYPES, 0),
                                                     "System",
                                                     FsSaveDataType_System)));

    // Should be good to go?
    return true;
}

static void load_application_records(void)
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
        s_titleInfoMap.emplace(std::make_pair(record.application_id, data::TitleInfo(record.application_id)));
    }
}

static void load_save_data_info(void)
{
    // Grab these here so I don't call the config functions every loop. Config uses a vector instead of map so the calls
    // can add up quickly.
    bool showAccountSystemSaves = config::get_by_key(config::keys::LIST_ACCOUNT_SYS_SAVES);
    bool onlyListMountable = config::get_by_key(config::keys::ONLY_LIST_MOUNTABLE);

    // Outer loop iterates through the save data spaces.
    for (int i = 0; i < 7; i++)
    {
        // Using fslib's save info reader wrapper.
        fslib::SaveInfoReader saveReader(SAVE_DATA_SPACE_ORDER[i]);
        // If it fails, log and continue the loop. Do not pass go or get 200 Monopoly fun bux.
        if (!saveReader)
        {
            logger::log(fslib::get_error_string());
            continue;
        }

        // Inner loop iterates save data info.
        while (saveReader.read())
        {
            // Grab a reference to the current SaveDataInfo struct.
            FsSaveDataInfo &saveInfo = saveReader.get();

            // This will filter out account system saves if desired and unmountable titles.
            if ((!showAccountSystemSaves && saveInfo.save_data_type == FsSaveDataType_System && saveInfo.uid != 0) ||
                (onlyListMountable && !fslib::open_save_data_with_save_info(fs::DEFAULT_SAVE_MOUNT, saveInfo)))
            {
                continue;
            }
            // If it made it here, we need to close the file system before we forget.
            fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT);

            // JKSV uses fake placeholder accounts for system type saves.
            AccountUid accountID = {0};
            switch (saveInfo.save_data_type)
            {
                case FsSaveDataType_Bcat:
                {
                    accountID = {FsSaveDataType_Bcat};
                }
                break;

                case FsSaveDataType_Device:
                {
                    accountID = {FsSaveDataType_Device};
                }
                break;

                case FsSaveDataType_Cache:
                {
                    accountID = {FsSaveDataType_Cache};
                }
                break;

                default:
                {
                    accountID = saveInfo.uid;
                }
                break;
            }

            // Find the user with the ID we have now.
            auto user = std::find_if(s_userVector.begin(), s_userVector.end(), [accountID](const UserIDPair &pair) {
                return accountID == pair.second.get_account_id();
            });

            // To do: Handle this like old JKSV did. Here we're just being quitters.
            if (user == s_userVector.end())
            {
                continue;
            }

            // System saves have no application ID.
            uint64_t applicationID = (saveInfo.save_data_type == FsSaveDataType_System ||
                                      saveInfo.save_data_type == FsSaveDataType_SystemBcat)
                                         ? saveInfo.system_save_data_id
                                         : saveInfo.application_id;

            // Search the map just to be sure it was loaded previously. This can happen.
            if (s_titleInfoMap.find(applicationID) == s_titleInfoMap.end())
            {
                s_titleInfoMap.emplace(std::make_pair(applicationID, data::TitleInfo(applicationID)));
            }

            // Grab a reference to the Title info so we don't try to load stats for titles that don't really exist.
            data::TitleInfo &titleInfo = s_titleInfoMap.at(applicationID);

            // I feel weird allcating space for this even if it's not used, but whatever.
            PdmPlayStatistics stats = {0};
            // This should be an OKish way to filter out system titles...
            if (titleInfo.has_control_data() &&
                R_FAILED(
                    pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(applicationID, accountID, false, &stats)))
            {
                // This isn't fatal.
                logger::log("Error getting play stats for title %016llX!", applicationID);
            }

            // Finally push it over to the user.
            user->second.add_data(saveInfo, stats);
        }
    }

    // Sort save info according to config.
    for (auto &[id, user] : s_userVector)
    {
        user.sort_data();
    }
}

static bool read_cache_file(void)
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

    // This should be the size of the cached control data.
    ssize_t cacheSize = titleCount * sizeof(CacheEntry);

    // Allocate array to read into.
    std::unique_ptr<CacheEntry[]> dataBuffer = std::make_unique<CacheEntry[]>(titleCount);

    // Read off the entire file in one shot.
    ssize_t readSize = cache.read(dataBuffer.get(), cacheSize);
    if (readSize != cacheSize)
    {
        logger::log("Cache size/read error!");
        return false;
    }

    // Loop through the cache entries and emplace them to the map.
    for (unsigned int i = 0; i < titleCount; i++)
    {
        // Grab a pointer to make this easier to read.
        CacheEntry *currentEntry = &dataBuffer[i];

        s_titleInfoMap.emplace(std::make_pair(currentEntry->m_applicationID, data::TitleInfo(currentEntry->m_data)));
    }

    return true;
}

static void create_cache_file(void)
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
}

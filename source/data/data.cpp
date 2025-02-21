#include "data/data.hpp"
#include "config.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include <algorithm>
#include <array>
#include <switch.h>
#include <unordered_map>
#include <vector>

namespace
{
    // This is easer to read imo
    using UserIDPair = std::pair<AccountUid, data::User>;
    // User vector to preserve order.
    std::vector<UserIDPair> s_userVector;
    // Map of Title info paired with its title/application
    std::unordered_map<uint64_t, data::TitleInfo> s_titleInfoMap;
    // Array of SaveDataSpaceIDs - SaveDataSpaceAll doesn't seem to work as it should...
    constexpr std::array<FsSaveDataSpaceId, 7> SAVE_DATA_SPACE_ORDER = {FsSaveDataSpaceId_System,
                                                                        FsSaveDataSpaceId_User,
                                                                        FsSaveDataSpaceId_SdSystem,
                                                                        FsSaveDataSpaceId_Temporary,
                                                                        FsSaveDataSpaceId_SdUser,
                                                                        FsSaveDataSpaceId_ProperSystem,
                                                                        FsSaveDataSpaceId_SafeMode};

} // namespace

bool data::initialize(void)
{
    // Switch can only have up to 8 accounts.
    int totalAccounts = 0;
    AccountUid accountIDs[8];
    Result accError = accountListAllUsers(accountIDs, 8, &totalAccounts);
    if (R_FAILED(accError))
    {
        logger::log("Error getting user list: 0x%X.", accError);
        return false;
    }

    // Loop through and load all users found.
    for (int i = 0; i < totalAccounts; i++)
    {
        data::User newUser(accountIDs[i], FsSaveDataType_Account);
        s_userVector.push_back(std::make_pair(accountIDs[i], newUser));
    }

    // Need this for save data of deleted users. It does happen. This is where they are inserted.
    // size_t userInsertPosition = s_userVector.size() - 1;

    // "System" user IDs.
    constexpr AccountUid deviceID = {FsSaveDataType_Device};
    constexpr AccountUid bcatID = {FsSaveDataType_Bcat};
    constexpr AccountUid cacheID = {FsSaveDataType_Cache};
    constexpr AccountUid systemID = {FsSaveDataType_System};

    // Push them to the user vector. Clang-format makes this look weird...
    s_userVector.push_back(std::make_pair(deviceID,
                                          data::User(deviceID,
                                                     strings::get_by_name(strings::names::SAVE_DATA_TYPES, 3),
                                                     "Device",
                                                     "romfs:/Textures/SystemSaves.png",
                                                     FsSaveDataType_Device)));
    s_userVector.push_back(std::make_pair(bcatID,
                                          data::User(bcatID,
                                                     strings::get_by_name(strings::names::SAVE_DATA_TYPES, 2),
                                                     "BCAT",
                                                     "romfs:/Textures/BCAT.png",
                                                     FsSaveDataType_Bcat)));
    s_userVector.push_back(std::make_pair(cacheID,
                                          data::User(cacheID,
                                                     strings::get_by_name(strings::names::SAVE_DATA_TYPES, 5),
                                                     "Cache",
                                                     "romfs:/Textures/Cache.png",
                                                     FsSaveDataType_Cache)));
    s_userVector.push_back(std::make_pair(systemID,
                                          data::User(systemID,
                                                     strings::get_by_name(strings::names::SAVE_DATA_TYPES, 0),
                                                     "System",
                                                     "romfs:/Textures/SystemSaves.png",
                                                     FsSaveDataType_System)));

    NsApplicationRecord currentRecord = {0};
    int entryCount = 0, entryOffset = 0;
    while (R_SUCCEEDED(nsListApplicationRecord(&currentRecord, 1, entryOffset++, &entryCount)) && entryCount > 0)
    {
        s_titleInfoMap.emplace(
            std::make_pair(currentRecord.application_id, data::TitleInfo(currentRecord.application_id)));
    }

    for (int i = 0; i < 7; i++)
    {
        fslib::SaveInfoReader saveInfoReader(SAVE_DATA_SPACE_ORDER[i]);
        if (!saveInfoReader)
        {
            logger::log(fslib::getErrorString());
            continue;
        }

        while (saveInfoReader.read())
        {
            FsSaveDataInfo &saveInfo = saveInfoReader.get();

            // This will filter out the account system saves unless the config is set to show them.
            if (!config::get_by_key(config::keys::LIST_ACCOUNT_SYS_SAVES) &&
                saveInfo.save_data_type == FsSaveDataType_System && saveInfo.uid != 0)
            {
                continue;
            }

            // Since JKSV uses fake users to support system type saves.
            AccountUid accountID;
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

            if (config::get_by_key(config::keys::ONLY_LIST_MOUNTABLE) &&
                !fslib::openSaveFileSystemWithSaveDataInfo(fs::DEFAULT_SAVE_MOUNT, saveInfo))
            {
                // Continue the loop since mounting failed.
                continue;
            }
            fslib::closeFileSystem(fs::DEFAULT_SAVE_MOUNT);

            // Find the user with the ID.
            auto findUser =
                std::find_if(s_userVector.begin(), s_userVector.end(), [accountID](const UserIDPair &userPair) {
                    return accountID == userPair.second.get_account_id();
                });

            if (findUser == s_userVector.end())
            {
                // To do: Handle this like old JKSV did.
                continue;
            }

            // This is for system save data since it has no application ID.
            uint64_t applicationID = (saveInfo.save_data_type == FsSaveDataType_System ||
                                      saveInfo.save_data_type == FsSaveDataType_SystemBcat)
                                         ? saveInfo.system_save_data_id
                                         : saveInfo.application_id;

            if (s_titleInfoMap.find(applicationID) == s_titleInfoMap.end())
            {
                s_titleInfoMap.emplace(std::make_pair(applicationID, data::TitleInfo(applicationID)));
            }

            PdmPlayStatistics playStats = {0};
            Result pdmError = pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(applicationID,
                                                                                       saveInfo.uid,
                                                                                       false,
                                                                                       &playStats);
            if (R_FAILED(pdmError))
            {
                // Logged, but not fatal.
                logger::log("Error getting play stats for %016llX: 0x%X", applicationID, pdmError);
            }
            // Push it to user.
            findUser->second.add_data(saveInfo, playStats);
        }
    }

    // Sort data for users.
    for (auto &[accountID, user] : s_userVector)
    {
        user.sort_data();
    }

    // Wew
    return true;
}

void data::get_users(std::vector<data::User *> &vectorOut)
{
    vectorOut.clear();
    for (auto &[accountID, userData] : s_userVector)
    {
        vectorOut.push_back(&userData);
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

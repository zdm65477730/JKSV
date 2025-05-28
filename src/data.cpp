#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>
#include <string_view>
#include <switch.h>
#include <unordered_map>
#include <vector>

#include "cfg.h"
#include "data.h"
#include "file.h"
#include "type.h"
#include "util.h"

namespace
{
    /// @brief This is the string used for when a publisher isn't found in the NACP.
    constexpr std::string_view STRING_UNKOWN_AUTHOR = "Someone?";

    /// @brief This array holds the order in which to scan save data space IDs since the all pseudo value from libnx doesn't seem to work.
    constexpr std::array<FsSaveDataSpaceId, 7> ORDER_SAVE_DATA_SPACES = {FsSaveDataSpaceId_System,
                                                                         FsSaveDataSpaceId_User,
                                                                         FsSaveDataSpaceId_SdSystem,
                                                                         FsSaveDataSpaceId_Temporary,
                                                                         FsSaveDataSpaceId_SdUser,
                                                                         FsSaveDataSpaceId_ProperSystem,
                                                                         FsSaveDataSpaceId_SafeMode};

    /// @brief This saves whether or not the System BCAT user was pushed.
    bool s_systemBCATPushed = false;

    /// @brief This saves whether or not the Temporary save user was pushed.
    bool s_temporaryPushed = false;
} // namespace

/// @brief External variables I used to track selected user/title.
int selUser = 0, selData = 0;

/// @brief This vector holds the user instances.
std::vector<data::user> data::users;

/// @brief System language. I don't know why this is here in data?
SetLanguage data::sysLang;

/// @brief This is the map of titleInfo.
std::unordered_map<uint64_t, data::titleInfo> data::titles;

//Sorts titles by sortType
static struct
{
        bool operator()(const data::userTitleInfo &a, const data::userTitleInfo &b)
        {
            //Favorites override EVERYTHING
            if (cfg::isFavorite(a.tid) != cfg::isFavorite(b.tid))
                return cfg::isFavorite(a.tid);

            switch (cfg::sortType)
            {
                case cfg::ALPHA:
                {
                    std::string titleA = data::getTitleNameByTID(a.tid);
                    std::string titleB = data::getTitleNameByTID(b.tid);
                    uint32_t pointA, pointB;
                    for (unsigned i = 0, j = 0; i < titleA.length();)
                    {
                        ssize_t aCnt = decode_utf8(&pointA, (const uint8_t *)&titleA.data()[i]);
                        ssize_t bCnt = decode_utf8(&pointB, (const uint8_t *)&titleB.data()[j]);
                        pointA = tolower(pointA), pointB = tolower(pointB);
                        if (pointA != pointB)
                            return pointA < pointB;

                        i += aCnt;
                        j += bCnt;
                    }
                }
                break;

                case cfg::MOST_PLAYED:
                    return a.playStats.playtime > b.playStats.playtime;
                    break;

                case cfg::LAST_PLAYED:
                    return a.playStats.last_timestamp_user > b.playStats.last_timestamp_user;
                    break;
            }
            return false;
        }
} sortTitles;

//Returns -1 for new
static int getUserIndex(AccountUid id)
{
    u128 nId = util::accountUIDToU128(id);
    for (unsigned i = 0; i < data::users.size(); i++)
        if (data::users[i].getUID128() == nId)
            return i;

    return -1;
}

static inline bool accountSystemSaveCheck(const FsSaveDataInfo &_inf)
{
    if (_inf.save_data_type == FsSaveDataType_System && util::accountUIDToU128(_inf.uid) != 0 &&
        !cfg::config["accSysSave"])
        return false;

    return true;
}

//Minimal init/test to avoid loading and creating things I don't need
static bool testMount(const FsSaveDataInfo &_inf)
{
    bool ret = false;
    if (!cfg::config["forceMount"])
        return true;

    if ((ret = fs::mountSave(_inf)))
        fs::unmountSave();

    return ret;
}

/// @brief This version of the function uses the title/application ID to pull the data from the system.
/// @param titleID Title/Application ID of the title to add.
static void add_title_to_list(uint64_t titleID)
{
    // This is the output size of the control data.
    uint64_t dataSize = 0;
    // This is the size of the icon.
    size_t iconSize = 0;
    // Nacp language entry pointer.
    NacpLanguageEntry *entry = nullptr;

    // Try to read the control data directly into the map.
    Result nsError = nsGetApplicationControlData(NsApplicationControlSource_Storage,
                                                 titleID,
                                                 &data::titles[titleID].data,
                                                 sizeof(NsApplicationControlData),
                                                 &dataSize);

    // At this point, regardless, the title should exist in the map. Using a reference from here on out.
    data::titleInfo &info = data::titles[titleID];

    // Calc icon size. Not sure this is really needed?
    iconSize = dataSize - sizeof(NacpStruct);

    // If that failed, run the backup routine and bail.
    if (R_FAILED(nsError) || dataSize < sizeof(info.data.nacp) || iconSize <= 0 ||
        R_FAILED(nacpGetLanguageEntry(&info.data.nacp, &entry)))
    {
        // Clear the NACP.
        std::memset(&info.data.nacp, 0x00, sizeof(NacpStruct));

        // Set the title and publisher.
        info.title = util::getIDStr(titleID);
        info.author = STRING_UNKOWN_AUTHOR;

        // Check if the title has a path defined in the config.
        if (cfg::isDefined(titleID))
        {
            info.safeTitle = cfg::getPathDefinition(titleID);
        }
        else
        {
            info.safeTitle = util::getIDStr(titleID);
        }

        // Create the icon.
        std::string idHex = util::getIDStrLower(titleID);
        info.icon = util::createIconGeneric(idHex.c_str(), 32, true);

        // Bail. Do not continue. We're done.
        return;
    }
    else
    {
        // Check to make sure the title isn't blank. If it is, use English.
        if (std::char_traits<char>::length(entry->name) == 0)
        {
            info.title = info.data.nacp.lang[SetLanguage_ENUS].name;
        }
        else
        {
            info.title = entry->name;
        }

        // Check the same for author/publisher.
        if (std::char_traits<char>::length(entry->author) == 0)
        {
            info.author = info.data.nacp.lang[SetLanguage_ENUS].author;
        }
        else
        {
            info.author = entry->author;
        }

        // Load the icon from the control data.
        info.icon = gfx::texMgr->textureLoadFromMem(IMG_FMT_JPG, info.data.icon, iconSize);

        // Make sure to save that the title has valid control data.
        info.hasControlData = true;
    }

    // The following is universal whether the title has control data or not.
    // Set the safe path title. Note: I hate the second else if statement, but screw it. Rewrite avoids this kind of stuff.
    if (cfg::isDefined(titleID))
    {
        info.safeTitle = cfg::getPathDefinition(titleID);
    }
    else if ((info.safeTitle = util::safeString(entry->name)) == "")
    {
        info.safeTitle = util::getIDStr(titleID);
    }

    // Favorite check.
    if (cfg::isFavorite(titleID))
    {
        info.fav = true;
    }
}

/// @brief Adds a title to the map using the application ID and cached control data.
/// @param titleID Application ID of the title.
/// @param data Reference to the data to use.
static void add_title_to_list(uint64_t titleID, NsApplicationControlData &data)
{
    // Start by memcpy'ing the data over.
    std::memcpy(&data::titles[titleID].data, &data, sizeof(NsApplicationControlData));

    // Since this function is always being fed cached data, we're going to assume everything else is good.
}


static inline bool titleIsLoaded(uint64_t titleID)
{
    return data::titles.find(titleID) != data::titles.end();
}

static void loadUserAccounts(void)
{
    // Total accounts listed.
    int total = 0;
    // Switch has eight accounts max.
    AccountUid accounts[8] = {0};

    if (R_FAILED(accountListAllUsers(accounts, 8, &total)))
    {
        // Rewrite logs this.
        return;
    }

    for (int i = 0; i < total; i++)
    {
        // Maybe to do: This looks stupid. Overload constructors?
        data::users.emplace_back(accounts[i], "", "");
    }
}

//This can load titles installed without having save data
static void loadTitlesFromRecords()
{
    NsApplicationRecord nsRecord;
    int32_t entryCount = 0, recordOffset = 0;
    while (R_SUCCEEDED(nsListApplicationRecord(&nsRecord, 1, recordOffset++, &entryCount)) && entryCount > 0)
    {
        if (!titleIsLoaded(nsRecord.application_id))
        {
            add_title_to_list(nsRecord.application_id);
        }
    }
}

static void import_svi_files(void)
{
    // This is the path used. JKSV master was written before fslib.
    std::string sviPath = fs::getWorkDir() + "svi";

    // Get the listing and check to make sure there was actually something in it.
    fs::dirList sviList(sviPath);
    if (sviList.getCount() <= 0)
    {
        // Just return and don't do anything.
        return;
    }

    // Loop through the listing and load them all.
    for (unsigned int i = 0; i < sviList.getCount(); i++)
    {
        // Full path to the file.
        std::string fullPath = sviPath + "/" + sviList.getItem(i);

        // Grab the size of the SVI before continuing.
        size_t sviSize = fs::fsize(fullPath);

        // Application ID
        uint64_t applicationID = 0;
        // Language entry.
        NacpLanguageEntry *entry = nullptr;
        // Icon size.
        size_t iconSize = 0;

        // Open the file and read the application ID.
        std::FILE *sviFile = std::fopen(sviPath.c_str(), "rb");
        std::fread(&applicationID, 1, sizeof(uint64_t), sviFile);

        // If it already exists in the map, just continue.
        if (titleIsLoaded(applicationID))
        {
            std::fclose(sviFile);
            continue;
        }

        // Read the NACP data into the map directly.
        std::fread(&data::titles[applicationID].data.nacp, 1, sizeof(NacpStruct), sviFile);

        // Calculate the icon size and read that too. The ControlData has memory set aside for the icon anyway.
        iconSize = fs::fsize(fullPath) - (sizeof(uint64_t) + sizeof(NacpStruct));
        std::fread(&data::titles[applicationID].data.icon, 1, iconSize, sviFile);

        // Think I should be safe to use a reference now?
        data::titleInfo &info = data::titles[applicationID];

        // Setup the title stuff.
        if (R_FAILED(nacpGetLanguageEntry(&info.data.nacp, &entry)))
        {
            // Default to English.
            info.title = info.data.nacp.lang[SetLanguage_ENUS].name;
            info.author = info.data.nacp.lang[SetLanguage_ENUS].author;
        }
        else
        {
            info.title = entry->name;
            info.author = entry->author;
        }

        // Safe path.
        if (cfg::isDefined(applicationID))
        {
            info.safeTitle = cfg::getPathDefinition(applicationID);
        }
        else if ((info.safeTitle = util::safeString(info.title)).empty())
        {
            info.safeTitle = util::getIDStr(applicationID);
        }

        // Favorite
        if (cfg::isFavorite(applicationID))
        {
            info.fav = true;
        }

        // Just going to assume this is good.
        info.icon = gfx::texMgr->textureLoadFromMem(IMG_FMT_JPG, info.data.icon, iconSize);
    }
}

bool data::loadUsersTitles(bool clearUsers)
{
    static unsigned systemUserCount = 4;
    FsSaveDataInfoReader it;
    FsSaveDataInfo info;
    s64 total = 0;

    loadTitlesFromRecords();
    import_svi_files();

    //Clear titles
    for (data::user &u : data::users)
    {
        u.titleInfo.clear();
    }
    if (clearUsers)
    {
        systemUserCount = 4;
        for (data::user &u : data::users)
        {
            u.delIcon();
        }
        data::users.clear();

        loadUserAccounts();
        s_systemBCATPushed = false;
        s_temporaryPushed = false;

        users.emplace_back(util::u128ToAccountUID(3), ui::getUIString("saveTypeMainMenu", 0), "Device");
        users.emplace_back(util::u128ToAccountUID(2), ui::getUIString("saveTypeMainMenu", 1), "BCAT");
        users.emplace_back(util::u128ToAccountUID(5), ui::getUIString("saveTypeMainMenu", 2), "Cache");
        users.emplace_back(util::u128ToAccountUID(0), ui::getUIString("saveTypeMainMenu", 3), "System");
    }

    for (unsigned i = 0; i < 7; i++)
    {
        if (R_FAILED(fsOpenSaveDataInfoReader(&it, ORDER_SAVE_DATA_SPACES.at(i))))
            continue;

        while (R_SUCCEEDED(fsSaveDataInfoReaderRead(&it, &info, 1, &total)) && total != 0)
        {
            uint64_t tid = 0;
            if (info.save_data_type == FsSaveDataType_System || info.save_data_type == FsSaveDataType_SystemBcat)
                tid = info.system_save_data_id;
            else
                tid = info.application_id;

            if (!titleIsLoaded(tid))
            {
                add_title_to_list(tid);
            }

            //Don't bother with this stuff
            if (cfg::isBlacklisted(tid) || !accountSystemSaveCheck(info) || !testMount(info))
                continue;

            switch (info.save_data_type)
            {
                case FsSaveDataType_Bcat:
                    info.uid = util::u128ToAccountUID(2);
                    break;

                case FsSaveDataType_Device:
                    info.uid = util::u128ToAccountUID(3);
                    break;

                case FsSaveDataType_SystemBcat:
                    info.uid = util::u128ToAccountUID(4);
                    if (!s_systemBCATPushed)
                    {
                        ++systemUserCount;
                        s_systemBCATPushed = true;
                        users.emplace_back(util::u128ToAccountUID(4),
                                           ui::getUIString("saveTypeMainMenu", 4),
                                           "System BCAT");
                    }
                    break;

                case FsSaveDataType_Cache:
                    info.uid = util::u128ToAccountUID(5);
                    break;

                case FsSaveDataType_Temporary:
                    info.uid = util::u128ToAccountUID(6);
                    if (!s_temporaryPushed)
                    {
                        ++systemUserCount;
                        s_temporaryPushed = true;
                        users.emplace_back(util::u128ToAccountUID(6),
                                           ui::getUIString("saveTypeMainMenu", 5),
                                           "Temporary");
                    }
                    break;
            }

            int u = getUserIndex(info.uid);
            if (u == -1)
            {
                users.emplace(data::users.end() - systemUserCount, info.uid, "", "");
                u = getUserIndex(info.uid);
            }

            PdmPlayStatistics playStats;
            if (info.save_data_type == FsSaveDataType_Account || info.save_data_type == FsSaveDataType_Device)
                pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(info.application_id,
                                                                         info.uid,
                                                                         false,
                                                                         &playStats);
            else
                memset(&playStats, 0, sizeof(PdmPlayStatistics));
            users[u].addUserTitleInfo(tid, &info, &playStats);
        }
        fsSaveDataInfoReaderClose(&it);
    }

    if (cfg::config["incDev"])
    {
        //Get reference to device save user
        unsigned devPos = getUserIndex(util::u128ToAccountUID(3));
        data::user &dev = data::users[devPos];
        for (unsigned i = 0; i < devPos; i++)
        {
            //Not needed but makes this easier to read
            data::user &u = data::users[i];
            u.titleInfo.insert(u.titleInfo.end(), dev.titleInfo.begin(), dev.titleInfo.end());
        }
    }

    data::sortUserTitles();

    return true;
}

void data::sortUserTitles()
{

    for (data::user &u : data::users)
        std::sort(u.titleInfo.begin(), u.titleInfo.end(), sortTitles);
}

void data::init()
{
    data::loadUsersTitles(true);
}

void data::exit()
{
    /*Still needed for planned future revisions*/
}

void data::setUserIndex(unsigned _sUser)
{
    selUser = _sUser;
}

data::user *data::getCurrentUser()
{
    return &users[selUser];
}

unsigned data::getCurrentUserIndex()
{
    return selUser;
}

void data::setTitleIndex(unsigned _sTitle)
{
    selData = _sTitle;
}

data::userTitleInfo *data::getCurrentUserTitleInfo()
{
    return &users[selUser].titleInfo[selData];
}

unsigned data::getCurrentUserTitleInfoIndex()
{
    return selData;
}

data::titleInfo *data::getTitleInfoByTID(const uint64_t &tid)
{
    if (titles.find(tid) != titles.end())
        return &titles[tid];
    return NULL;
}

std::string data::getTitleNameByTID(const uint64_t &tid)
{
    return titles[tid].title;
}

std::string data::getTitleSafeNameByTID(const uint64_t &tid)
{
    return titles[tid].safeTitle;
}

SDL_Texture *data::getTitleIconByTID(const uint64_t &tid)
{
    return titles[tid].icon;
}

int data::getTitleIndexInUser(const data::user &u, const uint64_t &tid)
{
    for (unsigned i = 0; i < u.titleInfo.size(); i++)
    {
        if (u.titleInfo[i].tid == tid)
            return i;
    }
    return -1;
}

data::user::user(AccountUid _id, const std::string &_backupName, const std::string &_safeBackupName)
{
    userID = _id;
    uID128 = util::accountUIDToU128(_id);

    AccountProfile prof;
    AccountProfileBase base;

    if (R_SUCCEEDED(accountGetProfile(&prof, userID)) && R_SUCCEEDED(accountProfileGet(&prof, NULL, &base)))
    {
        username = base.nickname;
        userSafe = util::safeString(username);
        if (userSafe.empty())
        {
            char tmp[32];
            sprintf(tmp, "Acc%08X", (uint32_t)uID128);
            userSafe = tmp;
        }

        uint32_t jpgSize = 0;
        accountProfileGetImageSize(&prof, &jpgSize);
        uint8_t *jpegData = new uint8_t[jpgSize];
        accountProfileLoadImage(&prof, jpegData, jpgSize, &jpgSize);
        userIcon = gfx::texMgr->textureLoadFromMem(IMG_FMT_JPG, jpegData, jpgSize);
        delete[] jpegData;

        accountProfileClose(&prof);
    }
    else
    {
        username = _backupName.empty() ? util::getIDStr((uint64_t)uID128) : _backupName;
        userSafe = _safeBackupName.empty() ? util::getIDStr((uint64_t)uID128) : _safeBackupName;
        userIcon = util::createIconGeneric(_backupName.c_str(), 48, false);
    }
    titles.reserve(64);
}

data::user::user(AccountUid _id, const std::string &_backupName, const std::string &_safeBackupName, SDL_Texture *img)
    : user(_id, _backupName, _safeBackupName)
{
    delIcon();
    userIcon = img;
    titles.reserve(64);
}

void data::user::setUID(AccountUid _id)
{
    userID = _id;
    uID128 = util::accountUIDToU128(_id);
}

void data::user::addUserTitleInfo(const uint64_t &tid, const FsSaveDataInfo *_saveInfo, const PdmPlayStatistics *_stats)
{
    data::userTitleInfo newInfo;
    newInfo.tid = tid;
    memcpy(&newInfo.saveInfo, _saveInfo, sizeof(FsSaveDataInfo));
    memcpy(&newInfo.playStats, _stats, sizeof(PdmPlayStatistics));
    titleInfo.push_back(newInfo);
}

static constexpr SDL_Color green = {0x00, 0xDD, 0x00, 0xFF};

void data::dispStats()
{
    data::user *cu = data::getCurrentUser();
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();

    //Easiest/laziest way to do this
    std::string stats = ui::getUICString("debugStatus", 0) + std::to_string(users.size()) + "\n";
    for (data::user &u : data::users)
        stats += u.getUsername() + ": " + std::to_string(u.titleInfo.size()) + "\n";
    stats += ui::getUICString("debugStatus", 1) + cu->getUsername() + "\n";
    stats += ui::getUICString("debugStatus", 2) + data::getTitleNameByTID(d->tid) + "\n";
    stats += ui::getUICString("debugStatus", 3) + data::getTitleSafeNameByTID(d->tid) + "\n";
    stats += ui::getUICString("debugStatus", 4) + std::to_string(cfg::sortType) + "\n";
    gfx::drawTextf(NULL, 16, 2, 2, &green, stats.c_str());
}

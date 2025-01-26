#include "data/User.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "data/data.hpp"
#include "logger.hpp"
#include "sdl.hpp"
#include "stringUtil.hpp"
#include <algorithm>
#include <cstring>

namespace
{
    /// @brief Font size for rendering text to icons.
    constexpr int ICON_FONT_SIZE = 50;
} // namespace

// Function used to sort user data.
static bool sortUserData(const data::UserDataEntry &entryA, const data::UserDataEntry &entryB)
{
    auto &[applicationIDA, dataA] = entryA;
    auto &[applicationIDB, dataB] = entryB;
    auto &[saveInfoA, playStatsA] = dataA;
    auto &[saveInfoB, playStatsB] = dataB;

    // Favorites over all.
    if (config::isFavorite(applicationIDA) != config::isFavorite(applicationIDB))
    {
        return config::isFavorite(applicationIDA);
    }

    data::TitleInfo *titleInfoA = data::getTitleInfoByID(applicationIDA);
    data::TitleInfo *titleInfoB = data::getTitleInfoByID(applicationIDB);
    switch (config::getByKey(config::keys::TITLE_SORT_TYPE))
    {
        // Alpha
        case 0:
        {
            // Get titles
            const char *titleA = titleInfoA->getTitle();
            const char *titleB = titleInfoB->getTitle();

            // Get the shortest of the two.
            size_t titleALength = std::char_traits<char>::length(titleA);
            size_t titleBLength = std::char_traits<char>::length(titleB);
            size_t shortestTitle = titleALength < titleBLength ? titleALength : titleBLength;
            // Loop and compare codepoints.
            for (size_t i = 0, j = 0; i < shortestTitle;)
            {
                // Decode UTF-8
                uint32_t codepointA = 0;
                uint32_t codepointB = 0;
                ssize_t unitCountA = decode_utf8(&codepointA, reinterpret_cast<const uint8_t *>(&titleA[i]));
                ssize_t unitCountB = decode_utf8(&codepointB, reinterpret_cast<const uint8_t *>(&titleB[j]));

                // Lower so case doesn't screw with it.
                int charA = std::tolower(codepointA);
                int charB = std::tolower(codepointB);
                if (charA != charB)
                {
                    return charA < charB;
                }

                i += unitCountA;
                j += unitCountB;
            }
        }
        break;

        // Most played.
        case 1:
        {
            return playStatsA.playtime > playStatsB.playtime;
        }
        break;

        // Last played.
        case 2:
        {
            return playStatsA.last_timestamp_user > playStatsB.last_timestamp_user;
        }
        break;
    }
    return false;
}

data::User::User(AccountUid accountID, FsSaveDataType saveType) : m_accountID(accountID), m_saveType(saveType)
{
    AccountProfile profile;
    AccountProfileBase profileBase = {0};

    // Whoever named these needs some help. What the hell?
    Result profileError = accountGetProfile(&profile, m_accountID);
    Result profileBaseError = accountProfileGet(&profile, NULL, &profileBase);
    if (R_FAILED(profileError) || R_FAILED(profileBaseError))
    {
        User::createAccount();
    }
    else
    {
        User::loadAccount(profile, profileBase);
    }
    accountProfileClose(&profile);
}

data::User::User(AccountUid accountID, std::string_view pathSafeNickname, std::string_view iconPath, FsSaveDataType saveType)
    : m_accountID(accountID), m_saveType(saveType), m_icon(sdl::TextureManager::createLoadTexture(pathSafeNickname, iconPath.data()))
{
    std::memcpy(m_pathSafeNickname, pathSafeNickname.data(), pathSafeNickname.length());
}

void data::User::addData(const FsSaveDataInfo &saveInfo, const PdmPlayStatistics &playStats)
{
    uint64_t applicationID = saveInfo.application_id == 0 ? saveInfo.system_save_data_id : saveInfo.application_id;

    m_userData.push_back(std::make_pair(applicationID, std::make_pair(saveInfo, playStats)));
}

void data::User::sortData(void)
{
    std::sort(m_userData.begin(), m_userData.end(), sortUserData);
}

AccountUid data::User::getAccountID(void) const
{
    return m_accountID;
}

FsSaveDataType data::User::getAccountSaveType(void) const
{
    return m_saveType;
}

const char *data::User::getNickname(void) const
{
    return m_nickname;
}

const char *data::User::getPathSafeNickname(void) const
{
    return m_pathSafeNickname;
}

size_t data::User::getTotalDataEntries(void) const
{
    return m_userData.size();
}

uint64_t data::User::getApplicationIDAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_userData.size()))
    {
        return 0;
    }
    return m_userData.at(index).first;
}

FsSaveDataInfo *data::User::getSaveInfoAt(int index)
{
    if (index < 0 || index >= static_cast<int>(m_userData.size()))
    {
        return nullptr;
    }
    return &m_userData.at(index).second.first;
}

PdmPlayStatistics *data::User::getPlayStatsAt(int index)
{
    if (index < 0 || index >= static_cast<int>(m_userData.size()))
    {
        return nullptr;
    }
    return &m_userData.at(index).second.second;
}

FsSaveDataInfo *data::User::getSaveInfoByID(uint64_t applicationID)
{
    auto findTitle = std::find_if(m_userData.begin(), m_userData.end(), [applicationID](data::UserDataEntry &entry) {
        return entry.first == applicationID;
    });

    if (findTitle == m_userData.end())
    {
        return nullptr;
    }
    return &findTitle->second.first;
}

PdmPlayStatistics *data::User::getPlayStatsByID(uint64_t applicationID)
{
    auto findTitle = std::find_if(m_userData.begin(), m_userData.end(), [applicationID](data::UserDataEntry &entry) {
        return entry.first == applicationID;
    });

    if (findTitle == m_userData.end())
    {
        return nullptr;
    }
    return &findTitle->second.second;
}

SDL_Texture *data::User::getIcon(void)
{
    return m_icon->get();
}

sdl::SharedTexture data::User::getSharedIcon(void)
{
    return m_icon;
}

void data::User::loadAccount(AccountProfile &profile, AccountProfileBase &profileBase)
{
    // Try to load icon.
    uint32_t iconSize = 0;
    Result accError = accountProfileGetImageSize(&profile, &iconSize);
    if (R_FAILED(accError))
    {
        logger::log("Error getting user icon size: 0x%X.", accError);
        User::createAccount();
        return;
    }

    std::unique_ptr<unsigned char[]> iconBuffer(new unsigned char[iconSize]);
    accError = accountProfileLoadImage(&profile, iconBuffer.get(), iconSize, &iconSize);
    if (R_FAILED(accError))
    {
        logger::log("Error loading user icon: 0x%08X.", accError);
        User::createAccount();
        return;
    }

    // We should be good at this point.
    m_icon = sdl::TextureManager::createLoadTexture(profileBase.nickname, iconBuffer.get(), iconSize);

    // Memcpy the nickname.
    std::memcpy(m_nickname, &profileBase.nickname, 0x20);

    if (!stringutil::sanitizeStringForPath(m_nickname, m_pathSafeNickname, 0x20))
    {
        std::string accountIDString = stringutil::getFormattedString("Account_%08X", m_accountID.uid[0] & 0xFFFFFFFF);
        std::memcpy(m_pathSafeNickname, accountIDString.c_str(), accountIDString.length());
    }
}

void data::User::createAccount(void)
{
    // This is needed a lot here.
    std::string accountIDString = stringutil::getFormattedString("Acc_%08X", m_accountID.uid[0] & 0xFFFFFFFF);

    // Create icon
    int textX = 128 - (sdl::text::getWidth(ICON_FONT_SIZE, accountIDString.c_str()) / 2);
    m_icon = sdl::TextureManager::createLoadTexture(accountIDString, 256, 256, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
    sdl::text::render(m_icon->get(),
                      textX,
                      128 - (ICON_FONT_SIZE / 2),
                      ICON_FONT_SIZE,
                      sdl::text::NO_TEXT_WRAP,
                      colors::WHITE,
                      accountIDString.c_str());

    // Memcpy the id string for both nicknames
    std::memcpy(m_nickname, accountIDString.c_str(), accountIDString.length());
    std::memcpy(m_pathSafeNickname, accountIDString.c_str(), accountIDString.length());
}

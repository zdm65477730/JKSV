#include "data/User.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "data/data.hpp"
#include "gfxutil.hpp"
#include "logger.hpp"
#include "sdl.hpp"
#include "stringutil.hpp"
#include <algorithm>
#include <cstring>

namespace
{
    /// @brief Font size for rendering text to icons.
    constexpr int SIZE_ICON_FONT = 50;
} // namespace

// Function used to sort user data. Definition at the bottom.
static bool sort_user_data(const data::UserDataEntry &entryA, const data::UserDataEntry &entryB);

data::User::User(AccountUid accountID, FsSaveDataType saveType) : m_accountID(accountID), m_saveType(saveType)
{
    AccountProfile profile;
    AccountProfileBase profileBase = {0};

    // Whoever named these needs some help. What the hell?
    Result profileError = accountGetProfile(&profile, m_accountID);
    Result profileBaseError = accountProfileGet(&profile, NULL, &profileBase);
    if (R_FAILED(profileError) || R_FAILED(profileBaseError))
    {
        User::create_account();
    }
    else
    {
        User::load_account(profile, profileBase);
    }
    accountProfileClose(&profile);
}

data::User::User(AccountUid accountID,
                 std::string_view nickname,
                 std::string_view pathSafeNickname,
                 FsSaveDataType saveType)
    : m_accountID(accountID), m_saveType(saveType)
{
    // Generate icon.
    m_icon = gfxutil::create_generic_icon(nickname, 48, colors::DIALOG_BOX, colors::WHITE);

    // We're just gonna use this for both.
    std::memcpy(m_nickname, nickname.data(), nickname.length());
    std::memcpy(m_pathSafeNickname, pathSafeNickname.data(), pathSafeNickname.length());
}

void data::User::add_data(const FsSaveDataInfo &saveInfo, const PdmPlayStatistics &playStats)
{
    uint64_t applicationID = saveInfo.application_id == 0 ? saveInfo.system_save_data_id : saveInfo.application_id;

    m_userData.push_back(std::make_pair(applicationID, std::make_pair(saveInfo, playStats)));
}

void data::User::clear_save_info(void)
{
    m_userData.clear();
}

void data::User::erase_data(int index)
{
    m_userData.erase(m_userData.begin() + index);
}

void data::User::sort_data(void)
{
    std::sort(m_userData.begin(), m_userData.end(), sort_user_data);
}

AccountUid data::User::get_account_id(void) const
{
    return m_accountID;
}

FsSaveDataType data::User::get_account_save_type(void) const
{
    return m_saveType;
}

const char *data::User::get_nickname(void) const
{
    return m_nickname;
}

const char *data::User::get_path_safe_nickname(void) const
{
    return m_pathSafeNickname;
}

size_t data::User::get_total_data_entries(void) const
{
    return m_userData.size();
}

uint64_t data::User::get_application_id_at(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_userData.size()))
    {
        return 0;
    }
    return m_userData.at(index).first;
}

FsSaveDataInfo *data::User::get_save_info_at(int index)
{
    if (index < 0 || index >= static_cast<int>(m_userData.size()))
    {
        return nullptr;
    }
    return &m_userData.at(index).second.first;
}

PdmPlayStatistics *data::User::get_play_stats_at(int index)
{
    if (index < 0 || index >= static_cast<int>(m_userData.size()))
    {
        return nullptr;
    }
    return &m_userData.at(index).second.second;
}

FsSaveDataInfo *data::User::get_save_info_by_id(uint64_t applicationID)
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

data::UserSaveInfoList &data::User::get_user_save_info_list(void)
{
    return m_userData;
}

PdmPlayStatistics *data::User::get_play_stats_by_id(uint64_t applicationID)
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

SDL_Texture *data::User::get_icon(void)
{
    return m_icon->get();
}

sdl::SharedTexture data::User::get_shared_icon(void)
{
    return m_icon;
}

void data::User::erase_save_info_by_id(uint64_t applicationID)
{
    auto targetEntry =
        std::find_if(m_userData.begin(), m_userData.end(), [applicationID](const data::UserDataEntry &entry) {
            return entry.second.first.application_id == applicationID;
        });

    if (targetEntry == m_userData.end())
    {
        // Do not pass go. Do not collect $200.
        return;
    }

    m_userData.erase(targetEntry);
}

void data::User::load_account(AccountProfile &profile, AccountProfileBase &profileBase)
{
    // Try to load icon.
    uint32_t iconSize = 0;
    Result accError = accountProfileGetImageSize(&profile, &iconSize);
    if (R_FAILED(accError))
    {
        logger::log("Error getting user icon size: 0x%X.", accError);
        User::create_account();
        return;
    }

    std::unique_ptr<unsigned char[]> iconBuffer(new unsigned char[iconSize]);
    accError = accountProfileLoadImage(&profile, iconBuffer.get(), iconSize, &iconSize);
    if (R_FAILED(accError))
    {
        logger::log("Error loading user icon: 0x%08X.", accError);
        User::create_account();
        return;
    }

    // We should be good at this point.
    m_icon = sdl::TextureManager::create_load_texture(profileBase.nickname, iconBuffer.get(), iconSize);

    // Memcpy the nickname.
    std::memcpy(m_nickname, &profileBase.nickname, 0x20);

    if (!stringutil::sanitize_string_for_path(m_nickname, m_pathSafeNickname, 0x20))
    {
        std::string accountIDString = stringutil::get_formatted_string("Account_%08X", m_accountID.uid[0] & 0xFFFFFFFF);
        std::memcpy(m_pathSafeNickname, accountIDString.c_str(), accountIDString.length());
    }
}

void data::User::create_account(void)
{
    // This is needed a lot here.
    std::string accountIDString = stringutil::get_formatted_string("Acc_%08X", m_accountID.uid[0] & 0xFFFFFFFF);

    // Create icon
    m_icon = gfxutil::create_generic_icon(accountIDString, SIZE_ICON_FONT, colors::DIALOG_BOX, colors::WHITE);

    // Memcpy the id string for both nicknames
    std::memcpy(m_nickname, accountIDString.c_str(), accountIDString.length());
    std::memcpy(m_pathSafeNickname, accountIDString.c_str(), accountIDString.length());
}

static bool sort_user_data(const data::UserDataEntry &entryA, const data::UserDataEntry &entryB)
{
    // Structured bindings to make this slightly more readable.
    auto &[applicationIDA, dataA] = entryA;
    auto &[applicationIDB, dataB] = entryB;
    auto &[saveInfoA, playStatsA] = dataA;
    auto &[saveInfoB, playStatsB] = dataB;

    // Favorites over all.
    if (config::is_favorite(applicationIDA) != config::is_favorite(applicationIDB))
    {
        return config::is_favorite(applicationIDA);
    }

    data::TitleInfo *titleInfoA = data::get_title_info_by_id(applicationIDA);
    data::TitleInfo *titleInfoB = data::get_title_info_by_id(applicationIDB);
    switch (config::get_by_key(config::keys::TITLE_SORT_TYPE))
    {
        // Alpha
        case 0:
        {
            // Get titles
            const char *titleA = titleInfoA->get_title();
            const char *titleB = titleInfoB->get_title();

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

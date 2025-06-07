#pragma once
#include "sdl.hpp"
#include <string>
#include <string_view>
#include <switch.h>
#include <vector>

namespace data
{
    /// @brief Type used to store save info and play statistics in the vector. Vector is used to preserve the order since I can't use a map without having to extra heap allocate it.
    using UserDataEntry = std::pair<uint64_t, std::pair<FsSaveDataInfo, PdmPlayStatistics>>;

    /// @brief Type definition for the user save info/play stats vector.
    using UserSaveInfoList = std::vector<UserDataEntry>;

    /// @brief Class that stores data for the user.
    class User
    {
        public:
            /// @brief Constructs a new user with accountID
            /// @param accountID AccountID of user.
            /// @param saveType Save data type account uses.
            User(AccountUid accountID, FsSaveDataType saveType);

            /// @brief This is the constructor used to create the fake system users.
            /// @param accountID AccountID to associate with saveType.
            /// @param pathSafeNickname The path safe version of the save data since JKSV is in everything the Switch supports.
            /// @param iconPath Path to the icon to load for account.
            /// @param saveType Save data type of user.
            User(AccountUid accountID,
                 std::string_view nickname,
                 std::string_view pathSafeNickname,
                 FsSaveDataType saveType);

            /// @brief Pushes data to m_userData
            /// @param saveInfo SaveDataInfo.
            /// @param playStats Play statistics.
            void add_data(const FsSaveDataInfo &saveInfo, const PdmPlayStatistics &playStats);

            /// @brief Clears the user save info vector.
            void clear_save_info(void);

            /// @brief Erases data at index.
            /// @param index Index of save data info to erase.
            void erase_data(int index);

            /// @brief Runs the sort algo on the vector.
            void sort_data(void);

            /// @brief Returns the account ID of the user.
            /// @return AccountID
            AccountUid get_account_id(void) const;

            /// @brief Returns the save data type the account uses.
            /// @return Save data type of the account.
            FsSaveDataType get_account_save_type(void) const;

            /// @brief Returns the account's nickname.
            /// @return Account nickname.
            const char *get_nickname(void) const;

            /// @brief Returns the path safe version of the nickname.
            /// @return Path safe nickname.
            const char *get_path_safe_nickname(void) const;

            /// @brief Returns the total number of entries in the data vector.
            /// @return Total number of entries.
            size_t get_total_data_entries(void) const;

            /// @brief Returns the application ID of the title at index.
            /// @param index Index of title.
            /// @return Application ID if index is valid. 0 if not.
            uint64_t get_application_id_at(int index) const;

            /// @brief Returns a pointer to the save data info at index.
            /// @param index Index of data to fetch.
            /// @return Pointer to info if valid. nullptr if out-of-bounds.
            FsSaveDataInfo *get_save_info_at(int index);

            /// @brief Returns a pointer to the play statistics at index.
            /// @param index Index of play statistics to fetch.
            /// @return Pointer to play statistics if index is value. nullptr if it's out of bounds.
            PdmPlayStatistics *get_play_stats_at(int index);

            /// @brief Returns a pointer to the save info of applicationID.
            /// @param applicationID Application ID to search and fetch for.
            /// @return Pointer to save info if found. nullptr if not.
            FsSaveDataInfo *get_save_info_by_id(uint64_t applicationID);

            /// @brief Returns a reference to the user save data info vector.
            /// @return Reference to the user save info vector.
            data::UserSaveInfoList &get_user_save_info_list(void);

            /// @brief Returns a pointer to the play statistics of applicationID
            /// @param applicationID Application ID to search and fetch.
            /// @return Pointer to play statistics if index is valid. nullptr if it isn't.
            PdmPlayStatistics *get_play_stats_by_id(uint64_t applicationID);

            /// @brief Returns raw SDL_Texture pointer of icon.
            /// @return SDL_Texture of icon.
            SDL_Texture *get_icon(void);

            /// @brief Returns the shared texture of icon. Increasing reference count of it.
            /// @return Shared icon texture.
            sdl::SharedTexture get_shared_icon(void);

            /// @brief Erases a UserDataEntry according to the application ID passed.
            /// @param applicationID ID of the save to erase.
            void erase_save_info_by_id(uint64_t applicationID);

        private:
            /// @brief Account's ID
            AccountUid m_accountID;

            /// @brief Type of save data account uses.
            FsSaveDataType m_saveType;

            /// @brief User's nickname.
            char m_nickname[0x20] = {0};

            /// @brief Path safe version of nickname.
            char m_pathSafeNickname[0x20] = {0};

            /// @brief User's icon.
            sdl::SharedTexture m_icon = nullptr;

            /// @brief Vector containing save info and play statistics.
            data::UserSaveInfoList m_userData;

            /// @brief Loads account structs from system.
            /// @param profile AccountProfile struct to write to.
            /// @param profileBase AccountProfileBase to write to.
            void load_account(AccountProfile &profile, AccountProfileBase &profileBase);

            /// @brief Creates a placeholder since something went wrong.
            void create_account(void);
    };
} // namespace data

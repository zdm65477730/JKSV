#pragma once
#include "data/TitleInfo.hpp"
#include "data/User.hpp"
#include "data/accountUID.hpp"

#include <unordered_map>
#include <vector>

namespace data
{
    /// @brief Declaration for user list/user pointer vector.
    using UserList      = std::vector<data::User *>;
    using TitleInfoList = std::vector<data::TitleInfo *>;

    /// @brief Initializes data. Loads user accounts from system and save data info.
    /// @param clearCache Whether or not the current cache file should be deleted from the SD card first.
    /// @return True on success. False on failure.
    bool initialize(bool clearCache);

    /// @brief Writes pointers to users to vectorOut
    /// @param userList List to push the pointers to.
    void get_users(data::UserList &userList);

    /// @brief Returns a pointer to the title mapped to applicationID.
    /// @param applicationID ApplicationID of title to retrieve.
    /// @return Pointer to data. nullptr if it's not found.
    data::TitleInfo *get_title_info_by_id(uint64_t applicationID);

    /// @brief Gets a vector of pointers to the title info.
    /// @param listOut List to store pointers in.
    void get_title_info_list(data::TitleInfoList &listOut);

    /// @brief Uses the application ID passed to add/load a title to the map.
    /// @param applicationID Application/System save data ID to add.
    void load_title_to_map(uint64_t applicationID);

    /// @brief Returns if the title with applicationID is already loaded to the map.
    /// @param applicationID Application ID of the title to search for.
    /// @return True if it has been. False if it hasn't.
    bool title_exists_in_map(uint64_t applicationID);

    /// @brief Gets a vector of pointers with all titles with saveType.
    /// @param saveType Save data type to check for.
    /// @param vectorOut Vector to push pointers to.
    void get_title_info_by_type(FsSaveDataType saveType, data::TitleInfoList &listOut);
} // namespace data

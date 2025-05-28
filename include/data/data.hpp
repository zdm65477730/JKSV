#pragma once
#include "data/TitleInfo.hpp"
#include "data/User.hpp"
#include "data/accountUID.hpp"
#include <unordered_map>
#include <vector>

namespace data
{
    /// @brief Declaration for user list/user pointer vector.
    using UserList = std::vector<data::User *>;

    /// @brief Loads users, applications, and save info from the system.
    /// @return True if everything goes fine. False if something goes horribly wrong.
    bool initialize(void);

    /// @brief Writes pointers to users to vectorOut
    /// @param userList List to push the pointers to.
    void get_users(data::UserList &userList);

    /// @brief Returns a pointer to the title mapped to applicationID.
    /// @param applicationID ApplicationID of title to retrieve.
    /// @return Pointer to data. nullptr if it's not found.
    data::TitleInfo *get_title_info_by_id(uint64_t applicationID);

    /// @brief Returns a reference to the title info map.
    /// @return Reference to TitleInfoMap.
    std::unordered_map<uint64_t, data::TitleInfo> &get_title_info_map(void);

    /// @brief Gets a vector of pointers with all titles with saveType.
    /// @param saveType Save data type to check for.
    /// @param vectorOut Vector to push pointers to.
    void get_title_info_by_type(FsSaveDataType saveType, std::vector<data::TitleInfo *> &vectorOut);
} // namespace data

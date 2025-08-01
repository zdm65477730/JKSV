#pragma once
#include "remote/Storage.hpp"

#include <memory>

namespace remote
{
    // Both of these are needed in two different places.
    static constexpr std::string_view PATH_GOOGLE_DRIVE_CONFIG = "sdmc:/config/JKSV/client_secret.json";
    static constexpr std::string_view PATH_WEBDAV_CONFIG       = "sdmc:/config/JKSV/webdav.json";

    /// @brief Returns whether or not the console has an active internet connection.
    bool has_internet_connection();

    /// @brief Initializes the Storage instance to Google Drive.
    void initialize_google_drive();

    /// @brief Initializes the Storage instance to WebDav
    void initialize_webdav();

    /// @brief Returns the pointer to the Storage instance.
    remote::Storage *get_remote_storage();
} // namespace remote

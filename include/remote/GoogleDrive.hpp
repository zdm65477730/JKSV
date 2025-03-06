#pragma once
#include "Storage.hpp"

namespace remote
{
    class GoogleDrive final : public remote::Storage
    {
        public:
            /// @brief Attempts to initialize a new instance of the GoogleDrive class.
            /// @param jsonPath Path to the client_secret file to read the information from.
            GoogleDrive(std::string_view jsonPath);

            /// @brief Destructor.
            ~GoogleDrive();

            /// @brief Creates a new directory on Google Drive.
            /// @param directoryName Name of the directory.
            /// @param parentId Parent of the directory.
            /// @return True on success. False on failure.
            bool create_directory(std::string_view directoryName, std::string_view parentId);

            /// @brief Uploads a file to Google drive.
            /// @param filename The name of the file.
            /// @param parentId The parent ID of the directory being uploaded to.
            /// @param target File to upload.
            /// @return True on success. False on failure.
            bool upload_file(std::string_view filename, std::string_view parentId, fslib::File &target);

            /// @brief Patches and updates a file on Google drive.
            /// @param fileId The ID of the file to patch.
            /// @param target File to patch/update from.
            /// @return True on success. False on failure.
            bool patch_file(std::string_view fileId, fslib::File &target);

            /// @brief Downloads a file from Google Drive.
            /// @param fileId ID of the file to download.
            /// @param target File to write downloaded to.
            /// @return True on success. False on failure.
            bool download_file(std::string_view fileId, fslib::File &target);

            /// @brief Deletes a file from Google Drive.
            /// @param fileId ID of the file to delete.
            /// @return True on success. False on failure.
            bool delete_file(std::string_view fileId);

        private:
            /// @brief Client ID.
            std::string m_clientId;

            /// @brief Client secret ID.
            std::string m_clientSecret;

            /// @brief Token.
            std::string m_token;

            /// @brief Refresh token.
            std::string m_refreshToken;

            /// @brief This holds the authorization bearer header.
            char m_authHeader[0x300] = {0};

            /// @brief These are the headers used.
            curl_slist *m_headers = nullptr;

            /// @brief Allows the user to sign in to Google Drive using the Switch's internal browser.
            /// @param authCodeOut String to write the authentication code to.
            /// @return True on success. False on failure.
            bool sign_in_and_authenticate(std::string &authCodeOut);

            /// @brief Exchanges the authentication code with the server.
            /// @param authCode Code to exchange.
            /// @return True on success. False on failure.
            bool exchange_auth_code(std::string_view authCode);

            /// @brief Refreshes the token.
            /// @return True on success. False on failure.
            bool refresh_token(void);

            /// @brief Checks if the token being used is still valid.
            /// @return True if token is still valid. False if it isn't.
            bool token_is_valid(void);

            /// @brief Uses listingUrl to request a listing.
            /// @param listingUrl URL to send the request to.
            /// @param jsonOut String to write the received data to.
            /// @return True on success. False on failure.
            bool request_listing(const char *listingUrl, std::string &jsonOut);

            /// @brief Processes the json of a listing request response.
            /// @param listingJson JSON to process.
            /// @param clear Whether or not the listing should be cleared before processing the listing.
            /// @return True on success. False on failure.
            bool process_listing(std::string_view listingJson, bool clear);

            /// @brief Checks the json for an error.
            /// @param json json_object to check.
            /// @return True if an error is found. False if not.
            /// @note Error is logged.
            bool error_occured(json_object *json);
    };
} // namespace remote

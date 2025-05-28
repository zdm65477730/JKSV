#pragma once
#include "json.hpp"
#include "remote/Storage.hpp"
#include <ctime>

namespace remote
{
    class GoogleDrive final : public remote::Storage
    {
        public:
            /// @brief Google Drive class constructor. Unlike PC prototype, the path to the client_secret is hardcoded.
            /// @param
            GoogleDrive(void);

            /// @brief Changes the current parent directory.
            /// @param id ID of the parent to target.
            void change_directory(std::string_view id) override;

            /// @brief Creates a new directory on Google Drive.
            /// @param name Name of the directory to create.
            /// @return True on success. False on failure.
            bool create_directory(std::string_view name) override;

            /// @brief Deletes a directory from Google Drive.
            /// @param id ID of the directory to delete.
            /// @return True on success. False on failure.
            bool delete_directory(std::string_view id) override;

            /// @brief Uploads a file to Google Drive.
            /// @param source Source path.
            /// @return True on success. False on failure.
            bool upload_file(const fslib::Path &source) override;

            /// @brief Patches or updates a file on Google Drive.
            /// @param id ID of the file to patch.
            /// @param source Source path of the updated file.
            /// @return True on success. False on failure.
            bool patch_file(std::string_view id, const fslib::Path &source) override;

            /// @brief Deletes a file from Google Drive.
            /// @param id ID of the file to delete.
            /// @return True on success. False on failure.
            bool delete_file(std::string_view id) override;

            /// @brief Downloads a file from Google Drive.
            /// @param id ID of the file to download.
            /// @param destination Path with the destination to save the file to.
            /// @return True on success. False on failure.
            bool download_file(std::string_view id, const fslib::Path &destination) override;

            // Sign in related functions.
            /// @brief Returns if a sign in is required for using Google Drive.
            /// @return True if sign in is required. False if it isn't.
            bool sign_in_required(void) const;

            /// @brief Gets the data needed to display and sign into Google.
            /// @param code String to write the sign in code to.
            /// @param expires std::time_t to write the expiration time to.
            /// @param wait Time in seconds while pinging server.
            /// @return True on success. False on failure.
            bool get_sign_in_data(std::string &code, std::time_t &expires, int &wait);

            /// @brief This function waits for the response from the server that the user signed in.
            /// @return True on success. False on failure.
            bool sign_in(void);


        private:
            /// @brief Client ID.
            std::string m_clientId;

            /// @brief Client secret.
            std::string m_clientSecret;

            /// @brief Authorization token.
            std::string m_token;

            /// @brief Token refresh token.
            std::string m_refreshToken;

            /// @brief Authentication header string.
            std::string m_authHeader;

            /// @brief Calculated time the token expires in.
            std::time_t m_tokenExpires;

            /// @brief Gets and sets the root ID of Google Drive using V2 of the API.
            /// @return True on success. False on failure.
            bool get_set_root_id(void);

            /// @brief Returns whether or not the token is still valid with a grace period of 10 seconds.
            /// @return True if the token is still valid. False if it isn't.
            bool token_is_valid(void) const;

            /// @brief Forces a refresh of the access token.
            /// @return True on success. False on failure.
            bool refresh_token(void);

            /// @brief Requests a listing and processes it.
            /// @return True on success. False on failure.
            bool request_listing(void);

            /// @brief Processes a file list from Google Drive.
            /// @param json Json object to process.
            /// @return True on success. False on failure.
            bool process_listing(json::Object &json);

            /// @brief Tries to locate a directory according to its ID instead of its name.
            /// @param id ID of the directory to search for.
            /// @return Iterator to directory found. m_list.end() on failure.
            remote::Storage::List::iterator find_directory_by_id(std::string_view id);

            /// @brief Tries to locate a file according to its ID instead of its name.
            /// @param id ID of the file to locate.
            /// @return Iterator to the file on success. m_list.end() on failure.
            remote::Storage::List::iterator find_file_by_id(std::string_view id);

            /// @brief Checks for, logs, and returns if an error is detected within the json::Object passed.
            /// @param json json::Object to check.
            /// @return True if an error was found. False if one wasn't.
            bool error_occurred(json::Object &json);
    };
} // namespace remote

#pragma once
#include "fslib.hpp"
#include "remote/StorageItem.hpp"
#include <curl/curl.h>
#include <string_view>
#include <vector>

namespace remote
{
    /// @brief Remote storage base class. All remote storage solutions must derive from this base class.
    class Storage
    {
        public:
            /// @brief Constructs a new storage instance.
            Storage(void);

            /// @brief Destructs the storage instance.
            virtual ~Storage();

            /// @brief Returns whether or not the remote service was initialized successfully.
            /// @return True if it was. False if not.
            bool is_initialized(void) const;

            /// @brief Returns if a directory exists with the parent and name passed.
            /// @param directory Directory to search for.
            /// @param parentId ID of the parent directory.
            /// @return True if it does. False on failure.
            bool directory_exists(std::string_view directory, std::string_view parentId);

            /// @brief Returns if a file exists with the parent id passed.
            /// @param fileName Name of the file to search for.
            /// @param parentId ID of the parent directory.
            /// @return True if it does. False if it doesn't.
            bool file_exists(std::string_view filename, std::string_view parentId);

            /// @brief Gets the id of the directory passed.
            /// @param directoryName Name of the directory to get the ID of.
            /// @param parentId Parent ID directory of the directory to get.
            /// @param idOut String to write the ID to.
            /// @return True if the directory is found. False if it doesn't.
            bool get_directory_id(std::string_view directoryName, std::string_view parentId, std::string &idOut);

            /// @brief Gets the ID of the file name passed.
            /// @param filename File name to search for.
            /// @param parentId ID of the parent directory.
            /// @param idOut String to write the ID to.
            /// @return True if a match is found. False if not.
            bool get_file_id(std::string_view filename, std::string_view parentId, std::string &idOut);

            /// @brief Virtual directory creation function.
            /// @param directoryName Name of the directory to create.
            /// @param parentId Parent ID of the parent directory.
            /// @return True on success. False on failure.
            virtual bool create_directory(std::string_view directoryName, std::string_view parentId) = 0;

            /// @brief Virtual function for uploading a file to the remote storage service.
            /// @param filename Name of the file on the remote storage service.
            /// @param parentId Parent ID of the directory to upload to.
            /// @param filePath File path of the local file to upload.
            /// @return True on success. False on failure.
            virtual bool upload_file(std::string_view filename,
                                     std::string_view parentId,
                                     const fslib::Path &filePath) = 0;

            /// @brief Virtual function for patching/updating a remote file.
            /// @param fileId ID of the file to patch.
            /// @param filePath Local path of the file to upload.
            /// @return True on success. False on failure.
            virtual bool patch_file(std::string_view fileId, const fslib::Path &filePath) = 0;

            /// @brief Virtual function for downloading a file from the remote storage service
            /// @param fileId ID of the file to download.
            /// @param filePath Local path to download the file to.
            /// @return True on success. False on failure.
            virtual bool download_file(std::string_view fileId, const fslib::Path &filePath) = 0;

            /// @brief Virtual function to delete a file from the remote storage service
            /// @param fileId ID of the file to delete.
            /// @return True on success. False on failure.
            virtual bool delete_file(std::string_view fileId) = 0;

        protected:
            /// @brief This stores whether or not logging in and initialization was successful.
            bool m_isInitialized = false;

            /// @brief Curl handle.
            CURL *m_curl = nullptr;

            /// @brief This vector holds the remote listing.
            std::vector<remote::StorageItem> m_remoteList;

            /// @brief Prepares the curl handle with base of a get request.
            void curl_prepare_get(void);

            /// @brief Prepares the curl handle with the base of a post.
            void curl_prepare_post(void);

            /// @brief Prepares the curl handle with a basic upload request.
            void curl_prepare_upload(void);

            /// @brief Prepares a patch request.
            void curl_prepare_patch(void);

            /// @brief Calls curl_easy_perform and returns a bool instead of curl error.
            /// @return True on success. False on failure.
            bool curl_perform(void);

        private:
            /// @brief Searches for and returns an iterator to the directory (if it's found).
            /// @param directoryName Directory to search for.
            /// @param parentId Parent ID of the directory.
            /// @return Iterator to directory if it's found. .end() if not?
            std::vector<remote::StorageItem>::iterator find_directory(std::string_view directoryName,
                                                                      std::string_view parentId);

            /// @brief Searches for and returns an iterator to the file (if it's found).
            /// @param filename Name of the file to search for.
            /// @param parentId Parent ID of the directory to search in.
            /// @return Iterator to file if found. .end() if not?
            std::vector<remote::StorageItem>::iterator find_file(std::string_view filename, std::string_view parentId);
    };
} // namespace remote

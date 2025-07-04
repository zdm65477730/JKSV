#pragma once
#include "curl/curl.hpp"
#include "fslib.hpp"
#include "remote/Item.hpp"
#include <ctime>
#include <string>
#include <vector>

namespace remote
{
    class Storage
    {
        public:
            /// @brief Definition to make things easier to type.
            using DirectoryListing = std::vector<remote::Item *>;

            /// @brief This makes writing some stuff for these classes way easier.
            using List = std::vector<remote::Item>;

            /// @brief This just allocates the curl::Handle.
            Storage();

            /// @brief Returns whether or not the Storage type was successfully. initialized.
            bool is_initialized() const;

            // Directory functions.
            /// @brief Returns whether or not a directory with name exists within the current parent.
            /// @param name Name of the directory to search for.
            bool directory_exists(std::string_view name);

            /// @brief Returns the parent to the root directory.
            void return_to_root();

            /// @brief This allows the root to be set to something other than what it originally was at construction.
            /// @param root Item to be used as the new root.
            void set_root_directory(remote::Item *root);

            /// @brief Changes the current parent directory.
            /// @param Item Item to use as the current parent directory.
            void change_directory(remote::Item *item);

            /// @brief Creates a directory in the current parent directory.
            /// @param name Name of the directory to create.
            virtual bool create_directory(std::string_view name) = 0;

            /// @brief Searches the list for a directory matching name and the current parent.
            /// @param name Name of the directory to search for.
            /// @return Pointer to the item representing the directory on success. nullptr on failure/not found.
            remote::Item *get_directory_by_name(std::string_view name);

            /// @brief Retrieves a listing of the items in the current parent directory.
            /// @param out List to fill.
            remote::Storage::DirectoryListing get_directory_listing();

            // File functions.
            /// @brief Returns whether a file with name exists within the current directory.
            /// @param name Name of the file.
            bool file_exists(std::string_view name);

            /// @brief Uploads a file from the SD card to the remote.
            /// @param source Path to the file to upload.
            virtual bool upload_file(const fslib::Path &source) = 0;

            /// @brief Patches or updates a file on the remote.
            /// @param item Item to be updated.
            /// @param source Path to the file to update with.
            virtual bool patch_file(remote::Item *file, const fslib::Path &source) = 0;

            /// @brief Downloads a file from the remote.
            /// @param item Item to download.
            /// @param destination Path to download the file to.
            virtual bool download_file(const remote::Item *file, const fslib::Path &destination) = 0;

            /// @brief Searches the list for a file matching name and the current parent.
            /// @param name Name of the file to search for.
            /// @return Pointer to the item if located. nullptr if not.
            remote::Item *get_file_by_name(std::string_view name);

            // General functions that apply to both.
            /// @brief Deletes a file or folder from the remote.
            /// @param item Item to delete.
            virtual bool delete_item(const remote::Item *item) = 0;

            /// @brief Returns whether or not the remote storage type supports UTF-8 for names or requires path safe titles.
            bool supports_utf8() const;

        protected:
            /// @brief This is the size of the buffers used for snprintf'ing URLs together.
            static constexpr size_t SIZE_URL_BUFFER = 0x401;

            /// @brief This is the size used for uploads.
            static constexpr size_t SIZE_UPLOAD_BUFFER = 0x10000;

            /// @brief This allows JKSV to know whether or not the storage type supports UTF-8.
            bool m_utf8Paths = false;

            /// @brief This stores whether or not the instance was initialized successfully.
            bool m_isInitialized = false;

            /// @brief This is the root directory of the remote storage.
            std::string m_root;

            /// @brief This stores the current parent.
            std::string m_parent;

            /// @brief Curl handle.
            curl::Handle m_curl;

            /// @brief This is the main remote listing.
            Storage::List m_list;

            /// @brief Searches the list for a directory matching name and the current parent.
            /// @param name Name to search for.
            Storage::List::iterator find_directory_by_name(std::string_view name);

            /// @brief Searches to find if a file with name exists within the current parent.
            /// @param name Name of the file to search for.
            Storage::List::iterator find_file_by_name(std::string_view name);
    };
} // namespace remote

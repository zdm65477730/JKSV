#pragma once
#include "fslib.hpp"
#include "remote/Item.hpp"
#include <string>
#include <vector>

namespace remote
{
    /// @brief This is the base storage class.
    class Storage
    {
        public:
            // Definition for remote file listing.
            using List = std::vector<remote::Item>;

            /// @brief Default storage constructor.
            Storage(void) = default;

            /// @brief Returns whether or not the storage type/driver was initialized successfully.
            /// @return True if it was. False if it wasn't.
            bool is_initialized(void) const;

            /// @brief Returns whether or not a directory with name exists within the current parent.
            /// @param name Name of the directory.
            /// @return True if one was found. False if one wasn't.
            bool directory_exists(std::string_view name);

            /// @brief Tries to locate and get the ID of a directory within the current parent.
            /// @param name Name of the directory to get.
            /// @param idOut String to write the ID to if it's found.
            /// @return True on success. False on failure.
            bool get_directory_id(std::string_view name, std::string &idOut);

            /// @brief Returns whether or not a file exists within the current parent directory.
            /// @param name Name of the file to search for.
            /// @return True if one is found. False if one isn't.
            bool file_exists(std::string_view name);

            /// @brief Tries to locate and get the ID of a file within the current parent directory.
            /// @param name Name of the file to search for
            /// @param idOut String it write the ID to.
            /// @return True on success. False when no file is found.
            bool get_file_id(std::string_view name, std::string &idOut);

            /// @brief Virtual function to change the current parent/working directory.
            /// @param name Name or ID of the directory to change to.
            virtual void change_directory(std::string_view name) = 0;

            /// @brief Virtual function to create a new directory within the current parent directory.
            /// @param name Name of the directory to create.
            /// @return True on success. False on failure.
            virtual bool create_directory(std::string_view name) = 0;

            /// @brief Virtual function to delete a directory from within the current parent directory.
            /// @param name Name or ID of the directory to delete.
            /// @return True on success. False on failure.
            virtual bool delete_directory(std::string_view name) = 0;

            /// @brief Virtual function to upload a file to remote storage.
            /// @param source fslib::File to use as the source to upload.
            /// @return True on success. False on failure.
            virtual bool upload_file(fslib::File &source) = 0;

            /// @brief Virtual function to patch or update a file.
            /// @param name Name or ID of the file to update.
            /// @param source Source file to use for the patch.
            /// @return True on success. False on failure.
            virtual bool patch_file(std::string_view name, fslib::File &source) = 0;

            /// @brief Virtual function to delete a file from the remote storage.
            /// @param name Name or ID of the file to delete.
            /// @return True on success. False on failure.
            virtual bool delete_file(std::string_view name) = 0;

        protected:
            /// @brief Whether or not initialization of the remote storage service was successful.
            bool m_isInitialized = false;

            /// @brief The root directory of the remote storage.
            std::string m_root;

            /// @brief Current parent directory.
            std::string m_parent;

            /// @brief Current storage listing.
            Storage::List m_list;

            /// @brief Searches for a directory in the current parent directory.
            /// @param name Name of the directory to search for.
            /// @return Iterator to found directory or m_list.end() on failure.
            Storage::List::iterator find_directory(std::string_view name);

            /// @brief Searches for a file named file within the current parent directory.
            /// @param name Name of the file to search for.
            /// @return Iterator to the file found. m_list.end() on failure.
            Storage::List::iterator find_file(std::string_view name);
    };
} // namespace remote

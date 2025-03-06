#pragma once
#include <string>

namespace remote
{
    /// @brief This class holds the information related to remote files on the storage service.
    class StorageItem
    {
        public:
            /// @brief Constructor for storage item.
            /// @param name File / Directory name.
            /// @param id ID of the item.
            /// @param parentId ID of the parent directory.
            /// @param size Size of the item.
            /// @param isDirectory Whether or not the item is a directory.
            StorageItem(std::string_view name,
                        std::string_view id,
                        std::string_view parentId,
                        int size,
                        bool isDirectory);

            /// @brief Function to set the size after patch/update.
            /// @param newSize New size of the item.
            void set_size(int newSize);

            /// @brief Returns the name of the item.
            /// @return Name of the item.
            std::string_view get_name(void) const;

            /// @brief Returns the ID of the item.
            /// @return ID of the item.
            std::string_view get_id(void) const;

            /// @brief Returns the ID of the parent.
            /// @return ID of the parent.
            std::string_view get_parent(void) const;

            /// @brief Returns the size of the item.
            /// @return Size of the item.
            int get_size(void) const;

            /// @brief Returns if the item is a directory.
            /// @return Whether or not the item is a directory.
            bool is_directory(void) const;

        private:
            /// @brief Item's name.
            std::string m_name;
            /// @brief Item's ID.
            std::string m_id;
            /// @brief ID of the parent.
            std::string m_parent;
            /// @brief Item's size.
            int m_size;
            /// @brief Whether or not the item is a directory.
            bool m_isDirectory;
    };
} // namespace remote

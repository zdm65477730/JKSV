#pragma once
#include <string>
#include <switch.h>

namespace fs
{
    class ScopedSaveMount
    {
        public:
            /// @brief Opens a scope save mount using the FsSaveDataInfo passed.
            /// @param mount Mount point.
            /// @param info Save info to mount.
            ScopedSaveMount(std::string_view mount, const FsSaveDataInfo *saveInfo);

            ScopedSaveMount(ScopedSaveMount &&scopedSaveMount);
            ScopedSaveMount &operator=(ScopedSaveMount &&scopedSaveMount);

            ScopedSaveMount(const ScopedSaveMount &)            = delete;
            ScopedSaveMount &operator=(const ScopedSaveMount &) = delete;

            /// @brief Closes the save mounted.
            ~ScopedSaveMount();

            /// @brief Returns whether or not mounting the data was successful.
            bool is_open() const;

        private:
            /// @brief Saves a copy of the mount point for destruction.
            std::string m_mountPoint{};

            /// @brief Stores whether or not mounting the save was successful.
            bool m_isOpen{};
    };
}

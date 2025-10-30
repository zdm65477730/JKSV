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
            /// @param log Optional. Whether or not logging errors is wanted. True by default.
            ScopedSaveMount(std::string_view mount, const FsSaveDataInfo *saveInfo, bool log = true);

            /// @brief Closes the save mounted.
            ~ScopedSaveMount();

            ScopedSaveMount(ScopedSaveMount &&scopedSaveMount) noexcept;
            ScopedSaveMount &operator=(ScopedSaveMount &&scopedSaveMount) noexcept;

            ScopedSaveMount(const ScopedSaveMount &)            = delete;
            ScopedSaveMount &operator=(const ScopedSaveMount &) = delete;

            /// @brief Returns whether or not mounting the data was successful.
            bool is_open() const noexcept;

        private:
            /// @brief Saves a copy of the mount point for destruction.
            std::string m_mountPoint{};

            /// @brief Stores whether or not mounting the save was successful.
            bool m_isOpen{};

            /// @brief Stores whether or not to log errors with the mount.
            bool m_log{};
    };
}

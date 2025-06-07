#pragma once
#include "sdl.hpp"
#include <cstdint>
#include <memory>
#include <switch.h>

namespace data
{
    /// @brief Class that holds data related to titles loaded from the system.
    class TitleInfo
    {
        public:
            /// @brief Constructs a TitleInfo instance. Loads control data, icon.
            /// @param applicationID Application ID of title to load.
            TitleInfo(uint64_t applicationID);

            /// @brief Initializes a TitleInfo instance using external (cached) NsApplicationControlData
            /// @param applicationID Application ID of the title loaded from cache.
            /// @param controlData Reference to the control data to init from.
            TitleInfo(uint64_t applicationID, NsApplicationControlData &controlData);

            /// @brief Returns the application ID of the title.
            /// @return Title's application ID.
            uint64_t get_application_id(void) const;

            /// @brief Returns a pointer to the control data for the title.
            /// @return Pointer to control data.
            NsApplicationControlData *get_control_data(void);

            /// @brief Returns whether or not the title has control data.
            /// @return Whether or not the title has control data.
            bool has_control_data(void) const;

            /// @brief Returns the title of the title?
            /// @return Title directly from the NACP.
            const char *get_title(void);

            /// @brief Returns the path safe version of the title for file system usage.
            /// @return Path safe version of the title.
            const char *get_path_safe_title(void);

            /// @brief Returns the publisher of the title.
            /// @return Publisher string from NACP.
            const char *get_publisher(void);

            /// @brief Returns the owner ID of the save data.
            /// @return Save data owner ID.
            uint64_t get_save_data_owner_id(void) const;

            /// @brief Returns the save data container's base size.
            /// @param saveType Type of save data to return.
            /// @return Size of baseline save data if applicable. If not, 0.
            int64_t get_save_data_size(uint8_t saveType) const;

            /// @brief Returns the maximum size of the save data container.
            /// @param saveType Type of save data to return.
            /// @return Maximum size of the save container if applicable. If not, 0.
            int64_t get_save_data_size_max(uint8_t saveType) const;

            /// @brief Returns the journaling size for the save type passed.
            /// @param saveType Save type to return.
            /// @return Journal size if applicable. If not, 0.
            int64_t get_journal_size(uint8_t saveType) const;

            /// @brief Returns the maximum journal size for the save type passed.
            /// @param saveType Save type to return.
            /// @return Maximum journal size if applicable. If not, 0.
            int64_t get_journal_size_max(uint8_t saveType) const;

            /// @brief Returns if a title uses the save type passed.
            /// @param saveType Save type to check for.
            /// @return True on success. False on failure.
            bool has_save_data_type(uint8_t saveType);

            /// @brief Returns a pointer to the icon texture.
            /// @return Icon
            sdl::SharedTexture get_icon(void) const;

            /// @brief Allows the path safe title to be set to a new path.
            /// @param newPathSafe Buffer containing the new safe path to use.
            /// @param newPathLength Size of the buffer passed.
            void set_path_safe_title(const char *newPathSafe, size_t newPathLength);

        private:
            /// @brief This defines how long the buffer is for the path safe version of the title.
            static inline constexpr size_t SIZE_PATH_SAFE = 0x200;

            /// @brief Stores application ID for easier grabbing since JKSV is all pointers.
            uint64_t m_applicationID = 0;

            /// @brief This contains the NACP and the icon.
            NsApplicationControlData m_data;

            /// @brief Saves whether or not the title has control data.
            bool m_hasData = false;

            /// @brief This is the path safe version of the title.
            char m_pathSafeTitle[TitleInfo::SIZE_PATH_SAFE] = {0};

            /// @brief Shared icon texture.
            sdl::SharedTexture m_icon = nullptr;

            /// @brief Private function to get/create the path safe title.
            void get_create_path_safe_title(void);
    };
} // namespace data

#pragma once
#include "sdl.hpp"
#include <cstdint>
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

            /// @brief Returns the application ID of the title.
            /// @return Title's application ID.
            uint64_t getApplicationID(void) const;

            /// @brief Returns the title of the title?
            /// @return Title directly from the NACP.
            const char *getTitle(void);

            /// @brief Returns the path safe version of the title for file system usage.
            /// @return Path safe version of the title.
            const char *getPathSafeTitle(void);

            /// @brief Returns the publisher of the title.
            /// @return Publisher string from NACP.
            const char *getPublisher(void);

            /// @brief Returns the owner ID of the save data.
            /// @return Save data owner ID.
            uint64_t getSaveDataOwnerID(void) const;

            /// @brief Returns the save data container's base size.
            /// @param saveType Type of save data to return.
            /// @return Size of baseline save data if applicable. If not, 0.
            int64_t getSaveDataSize(FsSaveDataType saveType) const;

            /// @brief Returns the maximum size of the save data container.
            /// @param saveType Type of save data to return.
            /// @return Maximum size of the save container if applicable. If not, 0.
            int64_t getSaveDataSizeMax(FsSaveDataType saveType) const;

            /// @brief Returns the journaling size for the save type passed.
            /// @param saveType Save type to return.
            /// @return Journal size if applicable. If not, 0.
            int64_t getJournalSize(FsSaveDataType saveType) const;

            /// @brief Returns the maximum journal size for the save type passed.
            /// @param saveType Save type to return.
            /// @return Maximum journal size if applicable. If not, 0.
            int64_t getJournalSizeMax(FsSaveDataType saveType) const;

            /// @brief Returns if a title uses the save type passed.
            /// @param saveType Save type to check for.
            /// @return True on success. False on failure.
            bool hasSaveDataType(FsSaveDataType saveType);

            /// @brief Returns a pointer to the icon texture.
            /// @return Icon
            sdl::SharedTexture getIcon(void) const;

        private:
            /// @brief Stores application ID for easier grabbing since JKSV is all pointers.
            uint64_t m_applicationID = 0;
            /// @brief This is where all the good stuff is. All the data for the title.
            NacpStruct m_nacp;
            /// @brief This is the path safe version of the title.
            char m_pathSafeTitle[0x200] = {0};
            /// @brief Shared icon texture.
            sdl::SharedTexture m_icon = nullptr;
    };
} // namespace data

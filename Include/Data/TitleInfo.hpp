#pragma once
#include "SDL.hpp"
#include <cstdint>
#include <switch.h>

namespace Data
{
    class TitleInfo
    {
        public:
            // Loads control data and icon.
            TitleInfo(uint64_t ApplicationID);

            // Returns title.
            const char *GetTitle(void);
            // Returns path safe title
            const char *GetPathSafeTitle(void);
            // Returns publisher
            const char *GetPublisher(void);
            // Returns the save data owner/application id
            uint64_t GetSaveDataOwnerID(void) const;
            // Returns save data size for save type. 0 on default.
            uint64_t GetSaveDataSize(FsSaveDataType SaveType) const;
            // Returns save data size max for save type. 0 on default.
            uint64_t GetSaveDataSizeMax(FsSaveDataType SaveType) const;
            // Returns journal size for given save type. 0 on default.
            uint64_t GetJournalSize(FsSaveDataType SaveType) const;
            // Returns the max journal size for save data type. 0 on default.
            uint64_t GetJournalSizeMax(FsSaveDataType SaveType) const;
            // Returns whether or not title has save data for type. Tests if NACP size is 0, basically.
            bool HasSaveDataType(FsSaveDataType SaveType);

            // Returns icon
            SDL::SharedTexture GetIcon(void) const;

        private:
            // This is where all the important stuff is.
            NacpStruct m_NACP;
            // This is the path safe version of the title.
            char m_PathSafeTitle[0x200] = {0};
            // This is the icon.
            SDL::SharedTexture m_Icon = nullptr;
    };
} // namespace Data

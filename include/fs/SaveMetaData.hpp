#pragma once
#include "data/TitleInfo.hpp"
#include <cstdint>
#include <switch.h>

namespace fs
{
    /// @brief This is the magic value written to the beginning.
    constexpr uint64_t SAVE_META_MAGIC = 0x56534B4A;

    /// @brief This is the filename used for the save data meta info.
    static constexpr std::string_view NAME_SAVE_META = ".jksv_save_meta.bin";

    /// @brief This struct is for storing the data necessary to restore saves to a different console.
    typedef struct __attribute__((packed))
    {
            /// @brief Magic.
            uint32_t m_magic;
            /// @brief Application ID.
            uint64_t m_applicationID;
            /// @brief Save data type.
            uint8_t m_saveType;
            /// @brief Save data rank.
            uint8_t m_saveRank;
            /// @brief Save data space id.
            uint8_t m_saveSpaceID;
            /// @brief Base save data size.
            int64_t m_saveDataSize;
            /// @brief Maximum save size "allowed".
            int64_t m_saveDataSizeMax;
            /// @brief Base journaling size.
            int64_t m_journalSize;
            /// @brief Maximum journaling size.
            int64_t m_journalSizeMax;
            /// @brief Total size of the files in the backup. For ZIP, this is uncompressed.
            uint64_t m_totalSaveSize;
    } SaveMetaData;

    /// @brief Didn't feel like a whole new file just for this. Fills an fs::SaveMetaData struct using the passed TitleInfo pointer.
    /// @param titleInfo TitleInfo instance to use to create the meta.
    /// @param info Reference to FsSaveDataInfo struct to use to fill out the meta struct.
    /// @param meta Struct to fill.
    void create_save_meta_data(data::TitleInfo *titleInfo, const FsSaveDataInfo *saveInfo, SaveMetaData &meta);
} // namespace fs

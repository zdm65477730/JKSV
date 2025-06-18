#pragma once
#include "data/TitleInfo.hpp"
#include <cstdint>
#include <switch.h>

namespace fs
{
    /// @brief This is the magic value written to the beginning.
    constexpr uint32_t SAVE_META_MAGIC = 0x56534B4A;

    /// @brief This is the filename used for the save data meta info.
    static constexpr std::string_view NAME_SAVE_META = ".nx_save_meta.bin";

    /// @brief Save data meta data struct.
    typedef struct __attribute__((packed))
    {
            /// @brief Meta file magic.
            uint32_t m_magic;
            /// @brief Meta revision.
            uint8_t m_revision;
            /// @brief Application ID of the game.
            uint64_t m_applicationID;
            /// @brief User account ID.
            AccountUid m_accountID;
            /// @brief System save data ID.
            uint64_t m_systemSaveID;
            /// @brief Save data type.
            uint8_t m_saveDataType;
            /// @brief Save data rank
            uint8_t m_saveDataRank;
            /// @brief Save data index. This only really used for cache saves.
            uint16_t m_saveDataIndex;
            // The rest of the attribute struct is useless, empty padding that is always 0?
            /// @brief Save data owner ID.
            uint64_t m_ownerID;
            /// @brief Just says timestamp. Not sure what time stamp.
            uint64_t m_timestamp;
            /// @brief Save Data flags.
            uint32_t m_flags;
            /// @brief Size of the save data.
            int64_t m_saveDataSize;
            /// @brief Save data's journal size.
            int64_t m_journalSize;
            /// @brief Commit ID.
            uint64_t m_commitID;
            // The rest of the struct is useless garbage padding.
    } SaveMetaData;

    /// @brief Didn't feel like a whole new file just for this. Fills an fs::SaveMetaData struct using the passed TitleInfo pointer.
    /// @param info Pointer to FsSaveDataInfo struct to use to fill out the meta struct.
    /// @param meta Struct to fill.
    bool fill_save_meta_data(const FsSaveDataInfo *saveInfo, SaveMetaData &meta);

    /// @brief Processes the save meta data and applies it to the passed saveInfo pointer.
    /// @param saveInfo FsSaveDataInfo to apply the meta to.
    /// @param meta Save meta data to apply.
    bool process_save_meta_data(const FsSaveDataInfo *saveInfo, const SaveMetaData &meta);

} // namespace fs

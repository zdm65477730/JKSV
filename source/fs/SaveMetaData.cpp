#include "fs/SaveMetaData.hpp"
#include "fs/directory_functions.hpp"
#include "fs/save_data_functions.hpp"
#include "fs/save_mount.hpp"
#include "fslib.hpp"
#include "logger.hpp"

namespace
{
    /// @brief This is the string template for errors here.
    constexpr std::string_view STRING_ERROR_TEMPLATE = "Error processing save meta for %016llX: %s";
} // namespace

bool fs::fill_save_meta_data(const FsSaveDataInfo *saveInfo, fs::SaveMetaData &meta)
{
    // This struct will allow us to fill this all out in one shot.
    FsSaveDataExtraData extraData;
    if (R_FAILED(fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(
            &extraData,
            sizeof(FsSaveDataExtraData),
            static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id),
            saveInfo->save_data_id)))
    {
        logger::log("Error generating save meta: Failed to read save extra data!");
        return false;
    }

    // Fill the struct.
    meta = {.m_magic = fs::SAVE_META_MAGIC,
            .m_revision = 0x00,
            .m_applicationID = extraData.attr.application_id,
            .m_accountID = extraData.attr.uid,
            .m_systemSaveID = extraData.attr.system_save_data_id,
            .m_saveDataType = extraData.attr.save_data_type,
            .m_saveDataRank = extraData.attr.save_data_rank,
            .m_saveDataIndex = extraData.attr.save_data_index,
            .m_ownerID = extraData.owner_id,
            .m_timestamp = extraData.timestamp,
            .m_flags = extraData.flags,
            .m_saveDataSize = extraData.data_size,
            .m_journalSize = extraData.journal_size,
            .m_commitID = extraData.commit_id};

    // Should be good.
    return true;
}

bool fs::process_save_meta_data(const FsSaveDataInfo *saveInfo, const SaveMetaData &meta)
{
    // We're going to grab this quick and use this to compare.
    FsSaveDataExtraData extraData = {0};
    if (R_FAILED(fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(
            &extraData,
            sizeof(FsSaveDataExtraData),
            static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id),
            saveInfo->save_data_id)))
    {
        logger::log(STRING_ERROR_TEMPLATE.data(), saveInfo->application_id, fslib::error::get_string());
        return false;
    }

    // We need to temporarily close the file system.
    if (!fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT))
    {
        logger::log(STRING_ERROR_TEMPLATE.data(), saveInfo->application_id, fslib::error::get_string());
        return false;
    }

    // To do: Other checks.
    if (extraData.data_size < meta.m_saveDataSize &&
        !fs::extend_save_data(saveInfo, meta.m_saveDataSize, meta.m_journalSize))
    {
        // The fs::extend_save_data function should log the error that occurred.
        return false;
    }

    // Now reopen it.
    if (!fslib::open_save_data_with_save_info(fs::DEFAULT_SAVE_MOUNT, *saveInfo))
    {
        logger::log(STRING_ERROR_TEMPLATE.data(), saveInfo->application_id, fslib::error::get_string());
        return false;
    }

    // More later if needed.
    return true;
}

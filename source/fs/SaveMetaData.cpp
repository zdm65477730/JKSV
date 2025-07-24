#include "fs/SaveMetaData.hpp"

#include "error.hpp"
#include "fs/directory_functions.hpp"
#include "fs/save_data_functions.hpp"
#include "fs/save_mount.hpp"
#include "fslib.hpp"

namespace
{
    constexpr size_t SIZE_EXTRA_DATA = sizeof(FsSaveDataExtraData);
}

bool fs::fill_save_meta_data(const FsSaveDataInfo *saveInfo, fs::SaveMetaData &meta)
{
    const FsSaveDataSpaceId spaceID = static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id);
    const uint64_t saveID           = saveInfo->save_data_id;

    FsSaveDataExtraData extraData{};
    const bool readError =
        error::libnx(fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(&extraData, SIZE_EXTRA_DATA, spaceID, saveID));
    if (readError) { return false; }

    meta = {.magic         = fs::SAVE_META_MAGIC,
            .revision      = 0x00,
            .applicationID = extraData.attr.application_id,
            .accountID     = extraData.attr.uid,
            .systemSaveID  = extraData.attr.system_save_data_id,
            .saveDataType  = extraData.attr.save_data_type,
            .saveDataRank  = extraData.attr.save_data_rank,
            .saveDataIndex = extraData.attr.save_data_index,
            .ownerID       = extraData.owner_id,
            .timestamp     = extraData.timestamp,
            .flags         = extraData.flags,
            .saveDataSize  = extraData.data_size,
            .journalSize   = extraData.journal_size,
            .commitID      = extraData.commit_id};

    return true;
}

bool fs::process_save_meta_data(const FsSaveDataInfo *saveInfo, const SaveMetaData &meta)
{
    const FsSaveDataSpaceId spaceID = static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id);
    const uint64_t saveID           = saveInfo->save_data_id;

    FsSaveDataExtraData extraData{};
    const bool readError =
        error::libnx(fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(&extraData, SIZE_EXTRA_DATA, spaceID, saveID));
    if (readError) { return false; }

    // We need to close this temporarily. To do: Look for a way to make this not needed?
    const bool closeError  = error::fslib(fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT));
    const bool needsExtend = extraData.data_size < meta.saveDataSize;
    const bool extended    = !closeError && needsExtend && fs::extend_save_data(saveInfo, meta.saveDataSize, meta.journalSize);
    if (needsExtend && !extended) { return false; }

    const bool reopenError = error::fslib(fslib::open_save_data_with_save_info(fs::DEFAULT_SAVE_MOUNT, *saveInfo));

    // Maybe more later.

    return reopenError;
}

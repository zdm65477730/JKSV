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

bool fs::read_save_data_extra_info(const FsSaveDataInfo *saveInfo, FsSaveDataExtraData &dataOut)
{
    const FsSaveDataSpaceId spaceID = static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id);
    const uint64_t saveDataID       = saveInfo->save_data_id;

    const bool readError =
        error::libnx(fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(&dataOut, SIZE_EXTRA_DATA, spaceID, saveDataID));
    if (readError) { return false; }
    return true;
}

bool fs::fill_save_meta_data(const FsSaveDataInfo *saveInfo, fs::SaveMetaData &meta)
{
    FsSaveDataExtraData extraData{};
    const bool extraRead = fs::read_save_data_extra_info(saveInfo, extraData);
    if (!extraRead) { return false; }

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
    FsSaveDataExtraData extraData{};
    const bool extraRead = fs::read_save_data_extra_info(saveInfo, extraData);
    if (!extraRead) { return false; }

    const bool needsExtend = extraData.data_size < meta.saveDataSize;
    const bool extended    = needsExtend && fs::extend_save_data(saveInfo, meta.saveDataSize, meta.journalSize);
    if (needsExtend && !extended) { return false; }

    return true;
}

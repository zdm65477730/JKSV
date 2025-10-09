#include "fs/save_data_functions.hpp"

#include "error.hpp"
#include "logging/logger.hpp"

namespace
{
    inline constexpr FsSaveDataMetaInfo SAVE_CREATE_META = {.size = 0x40060, .type = FsSaveDataMetaType_Thumbnail};
}

bool fs::create_save_data_for(data::User *targetUser,
                              data::TitleInfo *titleInfo,
                              uint16_t saveDataIndex,
                              uint8_t spaceID) noexcept
{
    // This is needed or creating BCAT saves will soft brick the Switch.
    static constexpr uint64_t ID_OWNER_BCAT = 0x010000000000000C;

    const uint8_t saveType       = targetUser->get_account_save_type();
    const uint64_t applicationID = titleInfo->get_application_id();
    const AccountUid accountID   = saveType == FsSaveDataType_Account ? targetUser->get_account_id() : data::BLANK_ACCOUNT_ID;
    const uint64_t ownerID       = saveType == FsSaveDataType_Bcat ? ID_OWNER_BCAT : titleInfo->get_save_data_owner_id();

    // Some cache type saves need this to create correctly.
    int64_t saveSize = titleInfo->get_save_data_size(saveType);
    saveSize         = saveSize == 0 ? titleInfo->get_save_data_size_max(saveType) : saveSize;

    int64_t journalSize = titleInfo->get_journal_size(saveType);
    journalSize         = journalSize == 0 ? titleInfo->get_journal_size_max(saveType) : journalSize;

    const FsSaveDataAttribute saveAttributes = {.application_id      = applicationID,
                                                .uid                 = accountID,
                                                .system_save_data_id = 0,
                                                .save_data_type      = saveType,
                                                .save_data_rank      = FsSaveDataRank_Primary,
                                                .save_data_index     = saveDataIndex};

    const FsSaveDataCreationInfo saveCreation = {.save_data_size     = saveSize,
                                                 .journal_size       = journalSize,
                                                 .available_size     = 0x4000,
                                                 .owner_id           = ownerID,
                                                 .flags              = 0,
                                                 .save_data_space_id = spaceID};

    // I want this recorded.
    return error::libnx(fsCreateSaveDataFileSystem(&saveAttributes, &saveCreation, &SAVE_CREATE_META)) == false;
}

bool fs::create_save_data_for(data::User *targetUser, const fs::SaveMetaData &saveMeta) noexcept
{
    const FsSaveDataAttribute saveAttributes = {.application_id      = saveMeta.applicationID,
                                                .uid                 = targetUser->get_account_id(),
                                                .system_save_data_id = saveMeta.systemSaveID,
                                                .save_data_type      = saveMeta.saveDataType,
                                                .save_data_rank      = saveMeta.saveDataRank,
                                                .save_data_index     = saveMeta.saveDataIndex};

    const FsSaveDataCreationInfo saveCreation = {.save_data_size     = saveMeta.saveDataSize,
                                                 .journal_size       = saveMeta.journalSize,
                                                 .available_size     = 0x4000,
                                                 .owner_id           = saveMeta.ownerID,
                                                 .flags              = 0,
                                                 .save_data_space_id = saveMeta.saveDataSpaceID};

    return error::libnx(fsCreateSaveDataFileSystem(&saveAttributes, &saveCreation, &SAVE_CREATE_META)) == false;
}

bool fs::delete_save_data(const FsSaveDataInfo *saveInfo) noexcept
{
    const FsSaveDataSpaceId spaceID = static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id);
    const bool isSystem             = fs::is_system_save_data(saveInfo);
    if (isSystem) { return false; }

    // Save attributes.
    const FsSaveDataAttribute saveAttributes = {.application_id      = saveInfo->application_id,
                                                .uid                 = saveInfo->uid,
                                                .system_save_data_id = saveInfo->system_save_data_id,
                                                .save_data_type      = saveInfo->save_data_type,
                                                .save_data_rank      = saveInfo->save_data_rank,
                                                .save_data_index     = saveInfo->save_data_index};

    return error::libnx(fsDeleteSaveDataFileSystemBySaveDataAttribute(spaceID, &saveAttributes)) == false;
}

bool fs::extend_save_data(const FsSaveDataInfo *saveInfo, int64_t size, int64_t journalSize)
{
    const FsSaveDataSpaceId spaceID = static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id);
    const uint64_t saveID           = saveInfo->save_data_id;

    return error::libnx(fsExtendSaveDataFileSystem(spaceID, saveID, size, journalSize)) == false;
}

bool fs::is_system_save_data(const FsSaveDataInfo *saveInfo) noexcept
{
    return saveInfo->save_data_type == FsSaveDataType_System || saveInfo->save_data_type == FsSaveDataType_SystemBcat;
}

bool fs::read_save_extra_data(const FsSaveDataInfo *saveInfo, FsSaveDataExtraData &extraOut) noexcept
{
    static constexpr size_t EXTRA_SIZE = sizeof(FsSaveDataExtraData);

    const FsSaveDataSpaceId spaceID = static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id);

    const bool readError = error::libnx(
        fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(&extraOut, EXTRA_SIZE, spaceID, saveInfo->save_data_id));

    return !readError;
}

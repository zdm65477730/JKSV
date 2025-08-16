#include "fs/save_data_functions.hpp"

#include "logging/error.hpp"
#include "logging/logger.hpp"

bool fs::create_save_data_for(data::User *targetUser, data::TitleInfo *titleInfo)
{
    static constexpr FsSaveDataMetaInfo saveMeta = {.size = 0x40060, .type = FsSaveDataMetaType_Thumbnail};

    const uint8_t saveType       = targetUser->get_account_save_type();
    const uint64_t applicationID = titleInfo->get_application_id();
    const AccountUid accountID   = saveType == FsSaveDataType_Account ? targetUser->get_account_id() : data::BLANK_ACCOUNT_ID;
    const uint64_t ownerID       = saveType == FsSaveDataType_Bcat ? 0x010000000000000C : titleInfo->get_save_data_owner_id();
    const int64_t saveSize       = titleInfo->get_save_data_size(saveType);
    const int64_t journalSize    = titleInfo->get_journal_size(saveType);

    const FsSaveDataAttribute saveAttributes = {.application_id      = applicationID,
                                                .uid                 = accountID,
                                                .system_save_data_id = 0,
                                                .save_data_type      = saveType,
                                                .save_data_rank      = FsSaveDataRank_Primary,
                                                .save_data_index     = 0};

    const FsSaveDataCreationInfo saveCreation = {.save_data_size     = saveSize,
                                                 .journal_size       = journalSize,
                                                 .available_size     = 0x4000,
                                                 .owner_id           = ownerID,
                                                 .flags              = 0,
                                                 .save_data_space_id = FsSaveDataSpaceId_User};

    // I want this recorded.
    const bool createError = error::libnx(fsCreateSaveDataFileSystem(&saveAttributes, &saveCreation, &saveMeta));
    return createError == false;
}

bool fs::delete_save_data(const FsSaveDataInfo *saveInfo)
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

    const bool deleteError = error::libnx(fsDeleteSaveDataFileSystemBySaveDataAttribute(spaceID, &saveAttributes));
    return deleteError == false;
}

bool fs::extend_save_data(const FsSaveDataInfo *saveInfo, int64_t size, int64_t journalSize)
{
    const FsSaveDataSpaceId spaceID = static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id);
    const uint64_t saveID           = saveInfo->save_data_id;

    const bool extendError = error::libnx(fsExtendSaveDataFileSystem(spaceID, saveID, size, journalSize));
    return extendError == false;
}

bool fs::is_system_save_data(const FsSaveDataInfo *saveInfo)
{
    return saveInfo->save_data_type == FsSaveDataType_System || saveInfo->save_data_type == FsSaveDataType_SystemBcat;
}

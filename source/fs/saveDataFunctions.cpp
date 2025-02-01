#include "fs/saveDataFunctions.hpp"
#include "logger.hpp"

bool fs::createSaveDataFor(data::User *targetUser, data::TitleInfo *titleInfo)
{
    // Attributes.
    FsSaveDataAttribute saveAttributes = {.application_id = titleInfo->getApplicationID(),
                                          .uid = targetUser->getAccountSaveType() == FsSaveDataType_Account ? targetUser->getAccountID()
                                                                                                            : data::BLANK_ACCOUNT_ID,
                                          .system_save_data_id = 0,
                                          .save_data_type = targetUser->getAccountSaveType(),
                                          .save_data_rank = FsSaveDataRank_Primary,
                                          .save_data_index = 0};

    FsSaveDataCreationInfo saveCreation = {
        .save_data_size = titleInfo->getSaveDataSize(targetUser->getAccountSaveType()),
        .journal_size = titleInfo->getJournalSize(targetUser->getAccountSaveType()),
        .available_size = 0x4000,
        .owner_id = targetUser->getAccountSaveType() == FsSaveDataType_Bcat ? 0x010000000000000C : titleInfo->getSaveDataOwnerID(),
        .flags = 0,
        .save_data_space_id = FsSaveDataSpaceId_User};

    // Save meta
    FsSaveDataMetaInfo saveMeta = {.size = 0x40060, .type = FsSaveDataMetaType_Thumbnail};

    Result fsError = fsCreateSaveDataFileSystem(&saveAttributes, &saveCreation, &saveMeta);
    if (R_FAILED(fsError))
    {
        logger::log("Error creating save data for %016llX: 0x%X.", titleInfo->getApplicationID(), fsError);
        return false;
    }
    return true;
}

bool fs::deleteSaveData(const FsSaveDataInfo &saveInfo)
{
    // I'm not allowing this at all.
    if (saveInfo.save_data_type == FsSaveDataType_System || saveInfo.save_data_type == FsSaveDataType_SystemBcat)
    {
        logger::log("Error deleting save data: Deleting system save data is not allowed.");
        return false;
    }

    // Save attributes.
    FsSaveDataAttribute saveAttributes = {.application_id = saveInfo.application_id,
                                          .uid = saveInfo.uid,
                                          .system_save_data_id = saveInfo.system_save_data_id,
                                          .save_data_type = saveInfo.save_data_type,
                                          .save_data_rank = saveInfo.save_data_rank,
                                          .save_data_index = saveInfo.save_data_index};

    Result fsError =
        fsDeleteSaveDataFileSystemBySaveDataAttribute(static_cast<FsSaveDataSpaceId>(saveInfo.save_data_space_id), &saveAttributes);
    if (R_FAILED(fsError))
    {
        logger::log("Error deleting save data: 0x%X.", fsError);
        return false;
    }
    return true;
}

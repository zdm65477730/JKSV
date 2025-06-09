#include "fs/save_data_functions.hpp"
#include "logger.hpp"

bool fs::create_save_data_for(data::User *targetUser, data::TitleInfo *titleInfo)
{
    // Attributes.
    FsSaveDataAttribute saveAttributes = {.application_id = titleInfo->get_application_id(),
                                          .uid = targetUser->get_account_save_type() == FsSaveDataType_Account
                                                     ? targetUser->get_account_id()
                                                     : data::BLANK_ACCOUNT_ID,
                                          .system_save_data_id = 0,
                                          .save_data_type = targetUser->get_account_save_type(),
                                          .save_data_rank = FsSaveDataRank_Primary,
                                          .save_data_index = 0};

    FsSaveDataCreationInfo saveCreation = {
        .save_data_size = titleInfo->get_save_data_size(targetUser->get_account_save_type()),
        .journal_size = titleInfo->get_journal_size(targetUser->get_account_save_type()),
        .available_size = 0x4000,
        .owner_id = targetUser->get_account_save_type() == FsSaveDataType_Bcat ? 0x010000000000000C
                                                                               : titleInfo->get_save_data_owner_id(),
        .flags = 0,
        .save_data_space_id = FsSaveDataSpaceId_User};

    // Save meta
    FsSaveDataMetaInfo saveMeta = {.size = 0x40060, .type = FsSaveDataMetaType_Thumbnail};

    Result fsError = fsCreateSaveDataFileSystem(&saveAttributes, &saveCreation, &saveMeta);
    if (R_FAILED(fsError))
    {
        logger::log("Error creating save data for %016llX: 0x%X.", titleInfo->get_application_id(), fsError);
        return false;
    }
    return true;
}

bool fs::delete_save_data(const FsSaveDataInfo *saveInfo)
{
    // I'm not allowing this at all.
    if (saveInfo->save_data_type == FsSaveDataType_System || saveInfo->save_data_type == FsSaveDataType_SystemBcat)
    {
        logger::log("Error deleting save data: Deleting system save data is not allowed.");
        return false;
    }

    // Save attributes.
    FsSaveDataAttribute saveAttributes = {.application_id = saveInfo->application_id,
                                          .uid = saveInfo->uid,
                                          .system_save_data_id = saveInfo->system_save_data_id,
                                          .save_data_type = saveInfo->save_data_type,
                                          .save_data_rank = saveInfo->save_data_rank,
                                          .save_data_index = saveInfo->save_data_index};

    Result fsError =
        fsDeleteSaveDataFileSystemBySaveDataAttribute(static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id),
                                                      &saveAttributes);
    if (R_FAILED(fsError))
    {
        logger::log("Error deleting save data: 0x%X.", fsError);
        return false;
    }
    return true;
}

bool fs::extend_save_data(const FsSaveDataInfo *saveInfo, int64_t size, int64_t journalSize)
{
    Result fsError = fsExtendSaveDataFileSystem(static_cast<FsSaveDataSpaceId>(saveInfo->save_data_space_id),
                                                saveInfo->save_data_id,
                                                size,
                                                journalSize);
    if (R_FAILED(fsError))
    {
        logger::log("Error extending save data: 0x%0X.", fsError);
        return false;
    }
    return true;
}

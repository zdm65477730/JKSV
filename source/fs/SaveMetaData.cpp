#include "fs/SaveMetaData.hpp"
#include "fs/directory_functions.hpp"
#include "fs/save_mount.hpp"

void fs::create_save_meta_data(data::TitleInfo *titleInfo, const FsSaveDataInfo *saveInfo, fs::SaveMetaData &meta)
{
    meta = {.m_magic = fs::SAVE_META_MAGIC,
            .m_applicationID = titleInfo->get_application_id(),
            .m_saveType = saveInfo->save_data_type,
            .m_saveRank = saveInfo->save_data_rank,
            .m_saveSpaceID = saveInfo->save_data_space_id,
            .m_saveDataSize = titleInfo->get_save_data_size(saveInfo->save_data_type),
            .m_saveDataSizeMax = titleInfo->get_journal_size_max(saveInfo->save_data_type),
            .m_journalSize = titleInfo->get_journal_size(saveInfo->save_data_type),
            .m_journalSizeMax = titleInfo->get_journal_size_max(saveInfo->save_data_type),
            .m_totalSaveSize = fs::get_directory_total_size(fs::DEFAULT_SAVE_ROOT)};
}

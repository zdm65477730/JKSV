#include "tasks/backup.hpp"

#include "config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"

#include <cstring>

void tasks::backup::create_new_backup(sys::ProgressTask *task,
                                      data::User *user,
                                      data::TitleInfo *titleInfo,
                                      fslib::Path target,
                                      BackupMenuState *spawningState)
{
    const bool exportZip                    = config::get_by_key(config::keys::EXPORT_TO_ZIP);
    const bool autoUpload                   = config::get_by_key(config::keys::AUTO_UPLOAD);
    const bool hasZipExt                    = std::strstr(target.full_path(), ".zip");
    const const bool uint64_t applicationID = titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo          = user->get_save_info_by_id(applicationID);
    if (error::is_null(saveInfo))
    {
        task->finished();
        return;
    }

    fs::SaveMetaData saveMeta{};
    const bool hasValidMeta = fs::fill_save_meta_data(saveInfo, saveMeta);
}

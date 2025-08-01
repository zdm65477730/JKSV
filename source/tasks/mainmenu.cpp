#include "tasks/mainmenu.hpp"

#include "config.hpp"
#include "data/data.hpp"
#include "error.hpp"
#include "remote/remote.hpp"
#include "stringutil.hpp"
#include "tasks/useroptions.hpp" // We're going to use the function from here.

void tasks::mainmenu::backup_all_for_all_users(sys::ProgressTask *task, MainMenuState::TaskData taskData)
{
    if (error::is_null(task)) { return; }

    remote::Storage *remote  = remote::get_remote_storage();
    const bool autoUpload    = config::get_by_key(config::keys::AUTO_UPLOAD);
    const bool exportZip     = config::get_by_key(config::keys::EXPORT_TO_ZIP);
    data::UserList &userList = taskData->userList;

    // Using this to make use of the user option functions.
    auto placeHolder           = std::make_shared<UserOptionState::DataStruct>();
    placeHolder->spawningState = nullptr;

    for (data::User *user : userList)
    {
        placeHolder->user = user;

        const int64_t titleCount = user->get_total_data_entries();
        for (int64_t i = 0; i < titleCount; i++)
        {
            const FsSaveDataInfo *saveInfo = user->get_save_info_at(i);
            if (error::is_null(saveInfo)) { continue; }

            const uint64_t applicationID = saveInfo->application_id;
            data::TitleInfo *titleInfo   = data::get_title_info_by_id(applicationID);
            if (error::is_null(titleInfo)) { continue; }

            const std::string name = user->get_path_safe_nickname() + " - " + stringutil::get_date_string();
            if (exportZip || autoUpload) { name += ".zip"; }

            if (remote && autoUpload) { tasks::useroptions::backup_all_for_user_remote(task, placeHolder, false); }
            else { tasks::useroptions::backup_all_for_user_local(task, placeHolder, false); }
        }
    }

    task->finished();
}

#include "tasks/savecreate.hpp"

#include "error.hpp"
#include "fs/fs.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"

void tasks::savecreate::create_save_data_for(sys::Task *task,
                                             data::User *user,
                                             data::TitleInfo *titleInfo,
                                             SaveCreateState *spawningState)
{
    if (error::is_null(task) || error::is_null(user) || error::is_null(titleInfo) || error::is_null(spawningState)) { return; }

    const int popTicks       = ui::PopMessageManager::DEFAULT_TICKS;
    const char *statusFormat = strings::get_by_name(strings::names::USEROPTION_STATUS, 0);
    const char *popSuccess   = strings::get_by_name(strings::names::SAVECREATE_POPS, 0);
    const char *popFailed    = strings::get_by_name(strings::names::SAVECREATE_POPS, 1);
    const char *title        = titleInfo->get_title();

    {
        const std::string status = stringutil::get_formatted_string(statusFormat, title);
        task->set_status(status);
    }

    const bool saveCreated = fs::create_save_data_for(user, titleInfo);
    if (!saveCreated) { ui::PopMessageManager::push_message(popTicks, popFailed); }
    else
    {
        const std::string popMessage = stringutil::get_formatted_string(popSuccess, title);
        ui::PopMessageManager::push_message(popTicks, popMessage);
    }

    spawningState->refresh_required();
    task->complete();
}

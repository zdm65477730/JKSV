#pragma once
#include "appstates/MainMenuState.hpp"
#include "sys/sys.hpp"

namespace tasks
{
    namespace mainmenu
    {
        void backup_all_for_all_local(sys::ProgressTask *task, MainMenuState::TaskData taskData);
        void backup_all_for_all_remote(sys::ProgressTask *task, MainMenuState::TaskData taskData);
    }
}

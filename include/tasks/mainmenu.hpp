#pragma once
#include "appstates/MainMenuState.hpp"
#include "sys/sys.hpp"

namespace tasks
{
    namespace mainmenu
    {
        void backup_all_for_all_users(sys::ProgressTask *task, MainMenuState::TaskData taskData);
    }
}

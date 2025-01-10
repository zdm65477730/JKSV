#pragma once
#include "AppStates/AppState.hpp"
#include "System/Task.hpp"
#include <switch.h>

class TaskState : public AppState
{
    public:
        template <typename... Args>
        TaskState(void (*Function)(System::Task *, Args...), Args... Arguments)
            : AppState(false), m_Task(Function, std::forward<Args>(Arguments)...)
        {
            appletBeginBlockingHomeButton(0);
        }

        ~TaskState()
        {
            appletEndBlockingHomeButton();
        }

        void Update(void);
        void Render(void);

    private:
        // Underlying task.
        System::Task m_Task;
};

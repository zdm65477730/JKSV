#pragma once
#include "AppStates/AppState.hpp"
#include "System/ProgressTask.hpp"
#include <string>
#include <switch.h>

class ProgressState : public AppState
{
    public:
        template <typename... Args>
        ProgressState(void (*Function)(System::ProgressTask *, Args...), Args... Arguments)
            : AppState(false), m_Task(Function, std::forward<Args>(Arguments)...)
        {
            appletBeginBlockingHomeButton(0);
        }

        ~ProgressState()
        {
            appletEndBlockingHomeButton();
        }

        void Update(void);
        void Render(void);

    private:
        // Underlying progress tracking task.
        System::ProgressTask m_Task;
        // Progress as whole number instead of decimal.
        size_t m_Progress = 0;
        // Width of the green bar. The other is hard coded for 720px.
        size_t m_ProgressBarWidth = 0;
        // X coordinate of the percentage.
        int m_PerentageX = 0;
        // String for percentage.
        std::string m_PercentageString;
};

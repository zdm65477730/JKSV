#pragma once
#include "AppStates/AppState.hpp"
#include "AppStates/ProgressState.hpp"
#include "AppStates/TaskState.hpp"
#include "Colors.hpp"
#include "Input.hpp"
#include "JKSV.hpp"
#include "SDL.hpp"
#include "System/Task.hpp"
#include "UI/RenderFunctions.hpp"
#include <memory>
#include <string>
#include <switch.h>
#include <tuple>

// To do: I didn't want to accomplish this with structs, but templating it was holding me up too much.
struct ConfirmStruct
{
};

template <typename TaskType, typename StateType>
class ConfirmState : public AppState
{
    public:
        // All functions using confirmation must follow this signature. The struct must be cast to the intended type for now.
        using TaskFunction = void (*)(TaskType *, std::shared_ptr<ConfirmStruct>);
        // Constructor
        ConfirmState(std::string_view QueryString, bool HoldRequired, TaskFunction Function, std::shared_ptr<ConfirmStruct> DataStruct)
            : m_QueryString(QueryString.data()), m_Hold(HoldRequired), m_Function(Function), m_DataStruct(DataStruct) {};

        ~ConfirmState() {};

        void Update(void)
        {
            if (Input::ButtonPressed(HidNpadButton_A))
            {
                AppState::Deactivate();
                JKSV::PushState(std::make_shared<StateType>(m_Function, m_DataStruct));
            }
            else if (Input::ButtonPressed(HidNpadButton_B))
            {
                AppState::Deactivate();
            }
        }

        void Render(void)
        {
            // Dim background
            SDL::RenderRectFill(NULL, 0, 0, 1280, 720, Colors::BackgroundDim);
            // Render dialog
            UI::RenderDialogBox(NULL, 280, 262, 720, 256);
            // Text
            SDL::Text::Render(NULL, 312, 288, 18, 656, Colors::White, m_QueryString.c_str());
            // Fake buttons. Maybe real later.
            SDL::RenderLine(NULL, 280, 454, 999, 454, Colors::White);
            SDL::RenderLine(NULL, 640, 454, 640, 517, Colors::White);
        }

    private:
        // Query string
        std::string m_QueryString;
        // Whether or not holding is required to confirm.
        bool m_Hold;
        // Function
        TaskFunction m_Function;
        // Shared ptr to data to send to confirmation function.
        std::shared_ptr<ConfirmStruct> m_DataStruct;
};

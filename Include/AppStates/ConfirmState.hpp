#pragma once
#include "AppStates/AppState.hpp"
#include "AppStates/ProgressState.hpp"
#include "AppStates/TaskState.hpp"
#include "Colors.hpp"
#include "Input.hpp"
#include "JKSV.hpp"
#include "SDL.hpp"
#include "Strings.hpp"
#include "System/Task.hpp"
#include "UI/RenderFunctions.hpp"
#include <memory>
#include <string>
#include <switch.h>

template <typename TaskType, typename StateType, typename StructType>
class ConfirmState : public AppState
{
    public:
        // All functions using confirmation must follow this signature.
        using TaskFunction = void (*)(TaskType *, std::shared_ptr<StructType>);
        // Constructor
        ConfirmState(std::string_view QueryString, bool HoldRequired, TaskFunction Function, std::shared_ptr<StructType> DataStruct)
            : AppState(false), m_QueryString(QueryString.data()), m_YesString(Strings::GetByName(Strings::Names::YesNo, 0)),
              m_Hold(HoldRequired), m_Function(Function), m_DataStruct(DataStruct)
        {
            appletBeginBlockingHomeButton(0);
        }

        ~ConfirmState()
        {
            appletEndBlockingHomeButton();
        }

        void Update(void)
        {
            if (Input::ButtonPressed(HidNpadButton_A) && !m_Hold)
            {
                AppState::Deactivate();
                JKSV::PushState(std::make_shared<StateType>(m_Function, m_DataStruct));
            }
            else if (Input::ButtonPressed(HidNpadButton_A) && m_Hold)
            {
                // Get the starting tick count and change the Yes string to the first holding string.
                m_StartingTickCount = SDL_GetTicks64();
                m_YesString = Strings::GetByName(Strings::Names::HoldingStrings, 0);
            }
            else if (Input::ButtonHeld(HidNpadButton_A) && m_Hold)
            {
                uint64_t TickCount = SDL_GetTicks64() - m_StartingTickCount;

                // If the TickCount is >= 3 seconds, confirmed. Else, just change the string so we can see we're not holding for nothing?
                if (TickCount >= 3000)
                {
                    AppState::Deactivate();
                    JKSV::PushState(std::make_shared<StateType>(m_Function, m_DataStruct));
                }
                else if (TickCount >= 2000)
                {
                    m_YesString = Strings::GetByName(Strings::Names::HoldingStrings, 2);
                }
                else if (TickCount >= 1000)
                {
                    m_YesString = Strings::GetByName(Strings::Names::HoldingStrings, 1);
                }
            }
            else if (Input::ButtonReleased(HidNpadButton_A))
            {
                m_YesString = Strings::GetByName(Strings::Names::YesNo, 0);
            }
            else if (Input::ButtonPressed(HidNpadButton_B))
            {
                // Just deactivate and don't do anything.
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
            // To do: Position this better. Currently brought over from old code.
            int YesX = 458 - SDL::Text::GetWidth(22, m_YesString.c_str());
            SDL::Text::Render(NULL, YesX, 478, 22, SDL::Text::NO_TEXT_WRAP, Colors::White, m_YesString.c_str());
            SDL::Text::Render(NULL, 782, 478, 22, SDL::Text::NO_TEXT_WRAP, Colors::White, Strings::GetByName(Strings::Names::YesNo, 1));
        }

    private:
        // Query string
        std::string m_QueryString;
        // Yes string.
        std::string m_YesString;
        // Whether or not holding is required to confirm.
        bool m_Hold;
        // For tick counting/holding
        uint64_t m_StartingTickCount = 0;
        // Function
        TaskFunction m_Function;
        // Shared ptr to data to send to confirmation function.
        std::shared_ptr<StructType> m_DataStruct;
};

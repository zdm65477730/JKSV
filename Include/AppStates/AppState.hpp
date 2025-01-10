#pragma once
#include "SDL.hpp"
#include <memory>

class AppState
{
    public:
        AppState(bool IsClosable = true) : m_IsClosable(IsClosable) {};
        virtual ~AppState() {};

        virtual void Update(void) = 0;
        virtual void Render(void) = 0;

        // Allows the state to be reactivated if needed.
        void Reactivate(void)
        {
            m_IsActive = true;
        }

        // Returns whether or not state is still active or can be purged.
        bool IsActive(void) const
        {
            return m_IsActive;
        }

        // Deactivates state and allows JKSV to kill it.
        void Deactivate(void)
        {
            m_IsActive = false;
        }

        // Tells state it has the current focus of the app.
        void GiveFocus(void)
        {
            m_HasFocus = true;
        }

        // Takes focus away from the state.
        void TakeFocus(void)
        {
            m_HasFocus = false;
        }

        // Returns whether state is at back of vector and has focus.
        bool HasFocus(void) const
        {
            return m_HasFocus;
        }

        // Returns whether or not state is closable with +.
        bool IsClosable(void) const
        {
            return m_IsClosable;
        }

    private:
        // Whether state is still active or can be purged.
        bool m_IsActive = true;
        // Whether or not state has focus
        bool m_HasFocus = false;
        // Whether or not state should allow exiting JKSV
        bool m_IsClosable = false;
};

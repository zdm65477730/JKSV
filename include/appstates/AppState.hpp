#pragma once
#include "sdl.hpp"
#include <memory>

class AppState
{
    public:
        /// @brief Base application state class.
        /// @param isClosable Optional. Controls whether or not the state should allow JKSV to close.
        AppState(bool isClosable = true);

        /// @brief Ends homebutton and plus locking.
        virtual ~AppState();

        /// @brief Every derived class is required to have this function.
        virtual void update(void) = 0;

        /// @brief Every derived class is required to have this function.
        virtual void render(void) = 0;

        /// @brief Deactivates state and allows JKSV to purge it from the vector.
        void deactivate(void);

        /// @brief Allows a state to be reactivated and pushed to the vector.
        void reactivate(void);

        /// @brief Returns if the state is still active.
        /// @return Whether state is still active or can be purged.
        bool isActive(void) const;

        /// @brief Tells the state it's at the back of the vector and has focus.
        void giveFocus(void);

        /// @brief Takes the focus away and tells the state it's no long back();
        void takeFocus(void);

        /// @brief Allows the state to know whether it has focus.
        /// @return Whether state has focus or not.
        bool hasFocus(void) const;

        /// @brief Returns whether or not JKSV should allow closing while state is active.
        /// @return True if closable. False if not.
        bool isClosable(void) const;

    private:
        /// @brief Stores whether or not the state is currently active.
        bool m_isActive = true;
        /// @brief Stores whether or not the state has focus.
        bool m_hasFocus = false;
        /// @brief Stores whether or not the state allows closing.
        bool m_isClosable = true;
};

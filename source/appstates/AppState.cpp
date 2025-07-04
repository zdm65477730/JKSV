#include "appstates/AppState.hpp"
#include <switch.h>

AppState::AppState(bool isClosable) : m_isClosable(isClosable)
{
    if (!m_isClosable)
    {
        appletBeginBlockingHomeButton(0);
    }
}

AppState::~AppState()
{
    if (!m_isClosable)
    {
        appletEndBlockingHomeButton();
    }
}

void AppState::deactivate()
{
    m_isActive = false;
}

void AppState::reactivate()
{
    m_isActive = true;
}

bool AppState::is_active() const
{
    return m_isActive;
}

void AppState::give_focus()
{
    m_hasFocus = true;
}

void AppState::take_focus()
{
    m_hasFocus = false;
}

bool AppState::has_focus() const
{
    return m_hasFocus;
}

bool AppState::is_closable() const
{
    return m_isClosable;
}

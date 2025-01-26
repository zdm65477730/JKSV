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

void AppState::deactivate(void)
{
    m_isActive = false;
}

void AppState::reactivate(void)
{
    m_isActive = true;
}

bool AppState::isActive(void) const
{
    return m_isActive;
}

void AppState::giveFocus(void)
{
    m_hasFocus = true;
}

void AppState::takeFocus(void)
{
    m_hasFocus = false;
}

bool AppState::hasFocus(void) const
{
    return m_hasFocus;
}

bool AppState::isClosable(void) const
{
    return m_isClosable;
}

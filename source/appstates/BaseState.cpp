#include "appstates/BaseState.hpp"

#include "error.hpp"

#include <switch.h>

BaseState::BaseState(bool isClosable)
    : m_isClosable(isClosable)
{
    if (m_isClosable) { return; }
    error::libnx(appletBeginBlockingHomeButtonShortAndLongPressed(0));
}

BaseState::~BaseState()
{
    if (m_isClosable) { return; }
    error::libnx(appletEndBlockingHomeButtonShortAndLongPressed());
}

void BaseState::deactivate() { m_isActive = false; }

void BaseState::reactivate() { m_isActive = true; }

bool BaseState::is_active() const { return m_isActive; }

void BaseState::give_focus() { m_hasFocus = true; }

void BaseState::take_focus() { m_hasFocus = false; }

bool BaseState::has_focus() const { return m_hasFocus; }

bool BaseState::is_closable() const { return m_isClosable; }

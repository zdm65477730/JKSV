#include "appstates/TaskCallbackState.hpp"

void TaskCallbackState::update() { m_callbackFunction(m_task.get()); }

void TaskCallbackState::render() { m_renderFunction(m_task.get()); }

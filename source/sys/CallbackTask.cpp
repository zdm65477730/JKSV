#include "sys/CallbackTask.hpp"

void sys::CallbackTask::task_complete() { m_taskFinished = true; }

void sys::CallbackTask::callback_complete() { m_callbackFinished = true; }

bool sys::CallbackTask::is_running() const { return m_taskFinished && m_callbackFinished; }

#include "StateManager.hpp"

void StateManager::update()
{
    // Grab the instance.
    StateManager &instance = StateManager::get_instance();

    if (instance.sm_stateVector.empty())
    {
        // Just return.
        return;
    }

    // Purge uneeded states.
    for (size_t i = instance.sm_stateVector.size() - 1; i > 0; i--)
    {
        // Grab a raw pointer to avoid reference count increase.
        AppState *appState = instance.sm_stateVector.at(i).get();

        if (!appState->is_active())
        {
            // Take focus first.
            appState->take_focus();
            instance.sm_stateVector.erase(instance.sm_stateVector.begin() + i);
        }
    }

    // Check if the back has focus. It should always have it.
    if (!instance.sm_stateVector.back()->has_focus())
    {
        instance.sm_stateVector.back()->give_focus();
    }

    // Only call update on the back.
    instance.sm_stateVector.back()->update();
}

void StateManager::render()
{
    // Instance.
    StateManager &instance = StateManager::get_instance();

    // Loop and render all states.
    for (std::shared_ptr<AppState> &appState : instance.sm_stateVector)
    {
        appState->render();
    }
}

bool StateManager::back_is_closable()
{
    // Instance.
    StateManager &instance = StateManager::get_instance();

    // Not too sure how to handle this yet.
    if (instance.sm_stateVector.empty())
    {
        return false;
    }

    // Just return this.
    return instance.sm_stateVector.back()->is_closable();
}

void StateManager::push_state(std::shared_ptr<AppState> newState)
{
    // Instance.
    StateManager &instance = StateManager::get_instance();

    // Take focus from the current back()
    if (!instance.sm_stateVector.empty())
    {
        instance.sm_stateVector.back()->take_focus();
    }

    // Give the incoming state focus and then push it.
    newState->give_focus();
    instance.sm_stateVector.push_back(newState);
}

StateManager &StateManager::get_instance()
{
    static StateManager instance;
    return instance;
}

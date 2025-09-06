#include "StateManager.hpp"

void StateManager::update()
{
    // Grab the instance.
    StateManager &instance = StateManager::get_instance();
    auto &stateVector      = instance.m_stateVector;

    if (stateVector.empty()) { return; }

    // Purge uneeded states.
    for (auto current = stateVector.begin(); current != stateVector.end();)
    {
        BaseState *state = current->get();

        if (!state->is_active())
        {
            state->take_focus();
            current = stateVector.erase(current);
            continue;
        }
        ++current;
    }

    if (stateVector.empty()) { return; }

    // Run sub update routines.
    for (auto &state : stateVector) { state->sub_update(); }

    std::shared_ptr<BaseState> &back = stateVector.back();
    if (!back->has_focus()) { back->give_focus(); }
    back->update();
}

void StateManager::render()
{
    StateManager &instance = StateManager::get_instance();
    auto &stateVector      = instance.m_stateVector;

    for (std::shared_ptr<BaseState> &state : stateVector) { state->render(); }
}

bool StateManager::back_is_closable()
{
    StateManager &instance = StateManager::get_instance();
    auto &stateVector      = instance.m_stateVector;

    if (stateVector.empty()) { return true; }

    std::shared_ptr<BaseState> &state = stateVector.back();
    return state->is_closable();
}

void StateManager::push_state(std::shared_ptr<BaseState> newState)
{
    StateManager &instance = StateManager::get_instance();
    auto &stateVector      = instance.m_stateVector;

    if (!stateVector.empty()) { stateVector.back()->take_focus(); }

    // Give the incoming state focus and then push it.
    newState->give_focus();
    stateVector.push_back(newState);
}

StateManager &StateManager::get_instance()
{
    static StateManager instance;
    return instance;
}

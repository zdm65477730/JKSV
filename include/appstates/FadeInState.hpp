#pragma once
#include "appstates/BaseState.hpp"
#include "system/Timer.hpp"

#include <memory>

class FadeInState final : public BaseState
{
    public:
        /// @brief Creates a new fade in state.
        /// @param nextState The next state to push after the the fade is finished.
        FadeInState(std::shared_ptr<BaseState> nextState);

        ~FadeInState() {};

        /// @brief Update override.
        void update() override;

        /// @brief Render override.
        void render() override;

    private:
        /// @brief Alpha value.
        uint8_t m_alpha = 0xFF;

        sys::Timer m_fadeTimer{};

        /// @brief Pointer to the next state to push.
        std::shared_ptr<BaseState> m_nextState{};
};

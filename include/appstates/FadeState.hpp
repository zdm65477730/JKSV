#pragma once
#include "appstates/BaseState.hpp"
#include "sdl.hpp"
#include "sys/sys.hpp"

#include <memory>

class FadeState final : public BaseState
{
    public:
        /// @brief Direction (In/Out) of the fade. This is auto-determined at construction.
        enum class Direction
        {
            In,
            Out
        };

        /// @brief Creates a new fade in state.
        /// @param nextState The next state to push after the the fade is finished.
        FadeState(sdl::Color baseColor, uint8_t startAlpha, uint8_t endAlpha, std::shared_ptr<BaseState> nextState);

        ~FadeState() {};

        /// @brief Returns a new fade in state. See constructor.
        static std::shared_ptr<FadeState> create(sdl::Color baseColor,
                                                 uint8_t startAlpha,
                                                 uint8_t endAlpha,
                                                 std::shared_ptr<BaseState> nextState);

        /// @brief Creates, returns and pushes a new FadeInState to the statemanager.
        static std::shared_ptr<FadeState> create_and_push(sdl::Color baseColor,
                                                          uint8_t startAlpha,
                                                          uint8_t endAlpha,
                                                          std::shared_ptr<BaseState> nextState);

        /// @brief Update override.
        void update() override;

        /// @brief Render override.
        void render() override;

    private:
        sdl::Color m_baseColor{};

        /// @brief Alpha value.
        uint8_t m_alpha{};

        /// @brief Alpha value to destruct at.
        uint8_t m_endAlpha{};

        FadeState::Direction m_direction{};

        uint8_t m_divisor{};

        /// @brief Timer for fade.
        sys::Timer m_fadeTimer{};

        /// @brief Pointer to the next state to push.
        std::shared_ptr<BaseState> m_nextState{};

        void find_divisor();
        void decrease_alpha();
        void increase_alpha();
        void completed();
};

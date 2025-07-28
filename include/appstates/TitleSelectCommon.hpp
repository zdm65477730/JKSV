#pragma once
#include "appstates/BaseState.hpp"

/// @brief Class that both view types are derived from.
class TitleSelectCommon : public BaseState
{
    public:
        /// @brief Constructs a new TitleSelectCommon. Basically just calculates the X coordinate of the control if it wasn't
        /// already.
        TitleSelectCommon();

        /// @brief Required destructor.
        virtual ~TitleSelectCommon() {};

        /// @brief Required, inherited.
        virtual void update() = 0;

        /// @brief Required, inherited.
        virtual void render() = 0;

        /// @brief Both derived classes need this function.
        virtual void refresh() = 0;

        /// @brief Renders the control guide string to the bottom right corner.
        void render_control_guide();

    private:
        /// @brief Pointer to the control guide string.
        static inline const char *sm_controlGuide{};

        /// @brief X coordinate the control guide is rendered at.
        static inline int sm_controlGuideX{};
};

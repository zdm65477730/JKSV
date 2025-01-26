#pragma once
#include "appstates/AppState.hpp"

/// @brief Class that both view types are derived from.
class TitleSelectCommon : public AppState
{
    public:
        /// @brief Constructs a new TitleSelectCommon. Basically just calculates the X coordinate of the control if it wasn't already.
        TitleSelectCommon(void);

        /// @brief Required destructor.
        virtual ~TitleSelectCommon() {};

        /// @brief Required, inherited.
        virtual void update(void) = 0;

        /// @brief Required, inherited.
        virtual void render(void) = 0;

        /// @brief Both derived classes need this function.
        virtual void refresh(void) = 0;

        /// @brief Renders the control guide string to the bottom right corner.
        void renderControlGuide(void);

    private:
        /// @brief X coordinate the control guide is rendered at.
        static inline int m_titleControlsX = 0;
};

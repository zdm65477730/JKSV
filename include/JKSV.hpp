#pragma once
#include "appstates/AppState.hpp"
#include "sdl.hpp"
#include <memory>
#include <vector>

/// @brief Main application class.
class JKSV
{
    public:
        /// @brief Initializes JKSV. Initializes services.
        JKSV(void);

        /// @brief Exits services.
        ~JKSV();

        /// @brief Returns if initializing was successful and JKSV is running.
        /// @return True or false.
        bool is_running(void) const;

        /// @brief Runs JKSV's update routine.
        void update(void);

        /// @brief Runs JKSV's render routine.
        void render(void);

    private:
        /// @brief Whether or not initialization was successful and JKSV is still running.
        bool m_isRunning = false;

        /// @brief Whether or not to print the translation credits.
        bool m_showTranslationInfo = false;

        /// @brief JKSV icon in upper left corner.
        sdl::SharedTexture m_headerIcon = nullptr;
};

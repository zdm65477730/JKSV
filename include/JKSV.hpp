#pragma once
#include "appstates/BaseState.hpp"
#include "sdl.hpp"

#include <memory>
#include <vector>

/// @brief Main application class.
class JKSV
{
    public:
        /// @brief Initializes JKSV. Initializes services.
        JKSV();

        /// @brief Exits services.
        ~JKSV();

        /// @brief Returns if initializing was successful and JKSV is running.
        /// @return True or false.
        bool is_running() const;

        /// @brief Runs JKSV's update routine.
        void update();

        /// @brief Runs JKSV's render routine.
        void render();

    private:
        /// @brief Whether or not initialization was successful and JKSV is still running.
        bool m_isRunning{};

        /// @brief Whether or not to print the translation credits.
        bool m_showTranslationInfo{};

        /// @brief JKSV icon in upper left corner.
        sdl::SharedTexture m_headerIcon{};

        /// @brief These are pointers to the translation info.
        const char *m_translation{};
        const char *m_author{};

        /// @brief Initializes fslib and takes care of a few other things.
        bool initialize_filesystem();

        /// @brief Initializes the services JKSV uses.
        bool initialize_services();

        // Creates the needed directories on SD.
        bool create_directories();

        /// @brief Adds the text color changing characters.
        void add_color_chars();

        /// @brief Exits all services.
        void exit_services();
};

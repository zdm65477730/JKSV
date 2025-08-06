#pragma once
#include "appstates/BaseState.hpp"
#include "sdl.hpp"
#include "ui/Menu.hpp"

/// @brief The state for settings.
class SettingsState final : public BaseState
{
    public:
        /// @brief Constructs a new settings state.
        SettingsState();

        /// @brief Required destructor.
        ~SettingsState() {};

        /// @brief Returns a new SettingsState.
        std::shared_ptr<SettingsState> create();

        /// @brief Runs the update routine.
        void update() override;

        /// @brief Runs the render routine.
        void render() override;

    private:
        /// @brief Menu for selecting and toggling settings.
        ui::Menu m_settingsMenu;

        /// @brief Pointer to the control guide string.
        const char *m_controlGuide{};

        // These are pointers to strings this state uses constantly.
        const char *m_onOff[2]{};
        const char *m_sortTypes[3]{};

        /// @brief X coordinate of the control guide in the bottom right corner.
        int m_controlGuideX{};

        /// @brief Render target to render to.
        sdl::SharedTexture m_renderTarget{};

        /// @brief Loads the settings menu strings.
        void load_settings_menu();

        /// @brief Loads the on/off and sort type strings.
        void load_extra_strings();

        /// @brief Runs a routine to update the menu strings for the menu.
        void update_menu_options();

        void change_working_directory();

        void create_push_blacklist_edit();

        /// @brief Toggles or executes the code to changed the selected menu option.
        void toggle_options();

        void cycle_zip_level();

        void cycle_sort_type();

        void toggle_jksm_mode();

        void cycle_anim_scaling();

        // Returns On/Off depending on the value passed.
        const char *get_status_text(uint8_t value);

        /// @brief Returns the sort type depending on the value passed.
        const char *get_sort_type_text(uint8_t value);
};

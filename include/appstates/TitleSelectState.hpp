#pragma once
#include "appstates/TitleSelectCommon.hpp"
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/TitleView.hpp"

/// @brief Title select state with icon tiles.
class TitleSelectState final : public TitleSelectCommon
{
    public:
        /// @brief Constructs new title select state.
        /// @param user User the state "belongs" to.
        TitleSelectState(data::User *user);

        /// @brief Required destructor.
        ~TitleSelectState() {};

        /// @brief Runs the update routine.
        void update() override;

        /// @brief Runs the render routine.
        void render() override;

        /// @brief Refreshes the view.
        void refresh() override;

    private:
        /// @brief Pointer to the user the view belongs to.
        data::User *m_user{};

        /// @brief Target to render to.
        sdl::SharedTexture m_renderTarget{};

        /// @brief Tiled title selection view.
        ui::TitleView m_titleView;

        /// @brief Checks if the user still has any games left to list. This is a safety measure.
        bool title_count_check();

        /// @brief Creates and pushes the backup menu state.
        void create_backup_menu();

        /// @brief Creates and pushes the title option state.
        void create_title_option_menu();

        /// @brief Resets and marks the state for purging.
        void deactivate_state();

        /// @brief Adds or removes the currently highlighted game from the favorite vector.
        void add_remove_favorite();
};

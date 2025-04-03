#pragma once
#include "appstates/TitleSelectCommon.hpp"
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/TitleView.hpp"

/// @brief Title select state with icon tiles.
class TitleSelectState : public TitleSelectCommon
{
    public:
        /// @brief Constructs new title select state.
        /// @param user User the state "belongs" to.
        TitleSelectState(data::User *user);

        /// @brief Required destructor.
        ~TitleSelectState() {};

        /// @brief Runs the update routine.
        void update(void) override;

        /// @brief Runs the render routine.
        void render(void) override;

        /// @brief Refreshes the view.
        void refresh(void) override;

    private:
        /// @brief Pointer to the user the view belongs to.
        data::User *m_user = nullptr;

        /// @brief Target to render to.
        sdl::SharedTexture m_renderTarget = nullptr;

        /// @brief Tiled title selection view.
        ui::TitleView m_titleView;
};

#pragma once
#include "appstates/TitleSelectCommon.hpp"
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/Menu.hpp"

/// @brief Text menu title selection state.
class TextTitleSelectState final : public TitleSelectCommon
{
    public:
        /// @brief Constructs new text menu title selection state.
        /// @param user User to construct title select for.
        TextTitleSelectState(data::User *user);

        /// @brief Required destructor.
        ~TextTitleSelectState() {};

        /// @brief Runs update routine.
        void update() override;

        /// @brief Runs render routine.
        void render() override;

        /// @brief Refreshes view for changes.
        void refresh() override;

    private:
        /// @brief Pointer to user view "belongs" to.
        data::User *m_user;

        /// @brief Menu to display titles to select from.
        ui::Menu m_titleSelectMenu;

        /// @brief Target to render to.
        sdl::SharedTexture m_renderTarget;
};

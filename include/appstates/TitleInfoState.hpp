#pragma once
#include "appstates/AppState.hpp"
#include "data/data.hpp"
#include "system/Timer.hpp"
#include "ui/SlideOutPanel.hpp"
#include "ui/TextScroll.hpp"
#include <memory>
#include <string>

class TitleInfoState : public AppState
{
    public:
        /// @brief Constructs a new title info state.
        /// @param user User to display info for.
        /// @param titleInfo Title to display info for.
        TitleInfoState(data::User *user, data::TitleInfo *titleInfo);

        /// @brief Required destructor.
        ~TitleInfoState();

        /// @brief Runs update routine.
        void update(void) override;

        /// @brief Runs render routine.
        void render(void) override;

    private:
        /// @brief Pointer to user.
        data::User *m_user;

        /// @brief Pointer to title info.
        data::TitleInfo *m_titleInfo;

        /// @brief This is a pointer to the title's icon.
        sdl::SharedTexture m_icon;

        /// @brief Bool to tell whether or not static members are initialized.
        static inline bool sm_initialized = false;

        /// @brief This is the render target for the title text just in case it needs scrolling.
        static inline sdl::SharedTexture sm_titleTarget = nullptr;

        /// @brief Slide panel.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel = nullptr;
};

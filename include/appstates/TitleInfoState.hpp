#pragma once
#include "appstates/AppState.hpp"
#include "data/data.hpp"
#include "system/Timer.hpp"
#include "ui/SlideOutPanel.hpp"
#include <memory>

class TitleInfoState : public AppState
{
    public:
        /// @brief Constructs a new title info state.
        /// @param user User to display info for.
        /// @param titleInfo Title to display info for.
        TitleInfoState(data::User *user, data::TitleInfo *titleInfo);

        /// @brief Required destructor.
        ~TitleInfoState() {};

        /// @brief Runs update routine.
        void update(void);

        /// @brief Runs render routine.
        void render(void);

    private:
        /// @brief Pointer to user.
        data::User *m_user;
        /// @brief Pointer to title info.
        data::TitleInfo *m_titleInfo;
        /// @brief Width of titles in pixels.
        int m_titleWidth = 0;
        /// @brief X coordinate of title text.
        int m_titleX = 0;
        /// @brief Whether or not the title is too long to fit in the panel and needs to be scrolled.
        bool m_titleScrolling = false;
        /// @brief Whether or not a scroll has been triggered.
        bool m_titleScrollTriggered = false;
        /// @brief Timer to scroll title if needed.
        sys::Timer m_titleScrollTimer;
        /// @brief Bool to tell whether or not static members are initialized.
        static inline bool sm_initialized = false;
        /// @brief Slide panel.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel = nullptr;
};

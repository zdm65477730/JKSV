#pragma once
#include "appstates/AppState.hpp"
#include "data/data.hpp"
#include "ui/Menu.hpp"
#include "ui/SlideOutPanel.hpp"
#include <memory>

class TitleOptionState : public AppState
{
    public:
        /// @brief Constructs a new title option state.
        /// @param user Target user.
        /// @param titleInfo Target title.
        TitleOptionState(data::User *user, data::TitleInfo *titleInfo);

        /// @brief Required destructor.
        ~TitleOptionState() {};

        /// @brief Runs update routine.
        void update(void) override;

        /// @brief Runs the render routine.
        void render(void) override;

    private:
        /// @brief This is just in case the option should only apply to the current user.
        data::User *m_user = nullptr;

        /// @brief This is the target title.
        data::TitleInfo *m_titleInfo = nullptr;

        /// @brief This is so it's known whether or not to initialize the static members of this class.
        static inline bool sm_initialized = false;

        /// @brief Menu used and shared by all instances.
        static inline std::unique_ptr<ui::Menu> sm_titleOptionMenu = nullptr;

        /// @brief This is shared by all instances of this class.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel = nullptr;
};

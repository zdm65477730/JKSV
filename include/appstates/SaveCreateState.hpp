#pragma once
#include "appstates/BaseState.hpp"
#include "appstates/TitleSelectCommon.hpp"
#include "data/data.hpp"
#include "ui/Menu.hpp"
#include "ui/SlideOutPanel.hpp"

#include <memory>

/// @brief This is the state that is spawned when CreateSaveData is selected from the user menu.
class SaveCreateState final : public BaseState
{
    public:
        /// @brief Constructs a new SaveCreateState.
        /// @param user The target user to create save data for.
        /// @param titleSelect The selection view for the user for refreshing and rendering.
        SaveCreateState(data::User *user, TitleSelectCommon *titleSelect);

        /// @brief Required destructor.
        ~SaveCreateState() {};

        /// @brief Runs the update routine.
        void update() override;

        /// @brief Runs the render routine.
        void render() override;

        /// @brief This signals so data and the view can be refreshed on the next update() to avoid threading shenanigans.
        void data_and_view_refresh_required();

    private:
        /// @brief Pointer to target user.
        data::User *m_user{};

        /// @brief Pointer to title selection view for the current user.
        TitleSelectCommon *m_titleSelect{};

        /// @brief Menu populated with every title found on the system.
        ui::Menu m_saveMenu;

        /// @brief Vector of pointers to the title info. This allows sorting them alphabetically and other things.
        std::vector<data::TitleInfo *> m_titleInfoVector{};

        /// @brief Whether or not a refresh is required on the next update() call.
        bool m_refreshRequired{};

        /// @brief Shared slide panel all instances use. There's no point in allocating a new one every time.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel{};
};

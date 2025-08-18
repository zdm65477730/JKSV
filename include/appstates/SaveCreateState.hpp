#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "appstates/TitleSelectCommon.hpp"
#include "data/data.hpp"
#include "ui/ui.hpp"

#include <atomic>
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

        /// @brief Returns a new SaveCreate state. See constructor for arguments.
        static inline std::shared_ptr<SaveCreateState> create(data::User *user, TitleSelectCommon *titleSelect)
        {
            return std::make_shared<SaveCreateState>(user, titleSelect);
        }

        /// @brief Creates, pushes, returns and new SaveCreateState.
        static inline std::shared_ptr<SaveCreateState> create_and_push(data::User *user, TitleSelectCommon *titleSelect)
        {
            auto newState = SaveCreateState::create(user, titleSelect);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Runs the update routine.
        void update() override;

        /// @brief Runs the render routine.
        void render() override;

        /// @brief This signals so data and the view can be refreshed on the next update() to avoid threading shenanigans.
        void refresh_required();

    private:
        /// @brief Pointer to target user.
        data::User *m_user{};

        /// @brief Pointer to title selection view for the current user.
        TitleSelectCommon *m_titleSelect{};

        /// @brief Menu populated with every title found on the system.
        std::shared_ptr<ui::Menu> m_saveMenu{};

        /// @brief Vector of pointers to the title info. This allows sorting them alphabetically and other things.
        std::vector<data::TitleInfo *> m_titleInfoVector{};

        /// @brief Whether or not a refresh is required on the next update() call.
        std::atomic<bool> m_refreshRequired{};

        /// @brief Shared slide panel all instances use. There's no point in allocating a new one every time.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel{};

        /// @brief Initializes static members if they haven't been already.
        void initialize_static_members();

        /// @brief Retrieves the data needed from data::
        void initialize_title_info_vector();

        /// @brief Pushes the titles to the menu
        void initialize_menu();

        /// @brief Launches the save creation task.
        void create_save_data_for();

        /// @brief Performs some operations and marks the state for purging.
        void deactivate_state();
};

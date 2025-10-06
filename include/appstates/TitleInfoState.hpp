#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "data/data.hpp"
#include "sys/sys.hpp"
#include "ui/ui.hpp"

#include <memory>
#include <string>
#include <vector>

class TitleInfoState final : public BaseState
{
    public:
        /// @brief Constructs a new title info state.
        /// @param user User to display info for.
        /// @param titleInfo Title to display info for.
        TitleInfoState(data::User *user, data::TitleInfo *titleInfo, const FsSaveDataInfo *saveInfo);

        /// @brief Creates a new TitleInfoState.
        static inline std::shared_ptr<TitleInfoState> create(data::User *user,
                                                             data::TitleInfo *titleInfo,
                                                             const FsSaveDataInfo *saveInfo)
        {
            return std::make_shared<TitleInfoState>(user, titleInfo, saveInfo);
        }

        /// @brief Creates, pushes, and returns a new TitleInfoState.
        static inline std::shared_ptr<TitleInfoState> create_and_push(data::User *user,
                                                                      data::TitleInfo *titleInfo,
                                                                      const FsSaveDataInfo *saveInfo)
        {
            auto newState = TitleInfoState::create(user, titleInfo, saveInfo);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Runs update routine.
        void update() override;

        /// @brief Runs render routine.
        void render() override;

    private:
        /// @brief Pointer to user.
        data::User *m_user{};

        /// @brief Pointer to title info.
        data::TitleInfo *m_titleInfo{};

        /// @brief Save a pointer to the save info.
        const FsSaveDataInfo *m_saveInfo{};

        /// @brief Whether or not the state was "closed";
        bool m_close{};

        /// @brief This is a pointer to the title's icon.
        sdl::SharedTexture m_icon{};

        /// @brief Transition for the open/close effect.
        ui::Transition m_transition{};

        /// @brief Stores whether or not the timer is started for the tiling animation.
        bool m_timerStarted{};

        /// @brief This is to create the "tiled in" effect because I like it.
        sys::Timer m_timer{};

        /// @brief Holds how many fields to display.
        int m_fieldDisplayCount{};

        /// @brief This is a switch to determine the clear color of the fields during creation.
        bool m_fieldClearSwitch{};

        /// @brief Vector of info scrolls.
        std::vector<std::shared_ptr<ui::TextScroll>> m_infoFields{};

        /// @brief This frame is shared by all instances. No point in allocating it over and over.
        static inline std::shared_ptr<ui::Frame> sm_frame{};

        /// @brief Initializes the static members if they haven't been already.
        void initialize_static_members();

        /// @brief Initializes and creates the scrolling/fields displaying information about the title/save.
        void initialize_info_fields();

        /// @brief This is always centered at the top and doesn't need an X
        void create_title(int y);

        /// @brief Creates and pushes the field for the application Id.
        void create_application_id(const FsSaveDataExtraData &extraData, int x, int &y);

        void create_save_data_id(int x, int &y);

        void create_owner_id(const FsSaveDataExtraData &extraData, int x, int &y);

        void create_size(const FsSaveDataExtraData &extraData, int x, int &y);

        void create_journal(const FsSaveDataExtraData &extraData, int x, int &y);

        void create_commit_id(const FsSaveDataExtraData &extraData, int x, int &y);

        void create_timestamp(const FsSaveDataExtraData &extraData, int x, int &y);

        void create_user(const FsSaveDataExtraData &extraData, int x, int &y);

        void create_first_played(const PdmPlayStatistics *playStats, int x, int &y);

        void create_last_played(const PdmPlayStatistics *playStats, int x, int &y);

        void create_play_time(const PdmPlayStatistics *playStats, int x, int &y);

        void create_launched(const PdmPlayStatistics *playStats, int x, int &y);

        void create_save_data_type(const FsSaveDataInfo *saveInfo, int x, int &y);

        /// @brief Updates the current count of fields to display.
        void update_field_count() noexcept;

        /// @brief Runs the update routine on all fields.
        void update_fields(bool hasFocus) noexcept;

        /// @brief Returns the color to clear the field with.
        sdl::Color get_field_color() noexcept;

        /// @brief Signals to close the state.
        void close() noexcept;

        /// @brief Performs some operations and marks the state for purging.
        void deactivate_state() noexcept;
};

#pragma once
#include "appstates/BaseState.hpp"
#include "appstates/TitleSelectCommon.hpp"
#include "data/data.hpp"
#include "ui/Menu.hpp"
#include "ui/SlideOutPanel.hpp"

#include <memory>

class TitleOptionState final : public BaseState
{
    public:
        /// @brief Constructs a new title option state.
        /// @param user Target user.
        /// @param titleInfo Target title.
        TitleOptionState(data::User *user, data::TitleInfo *titleInfo, TitleSelectCommon *titleSelect);

        /// @brief Required destructor.
        ~TitleOptionState() {};

        /// @brief Returns a new TitleOptionState. See constructor.
        static std::shared_ptr<TitleOptionState> create(data::User *user,
                                                        data::TitleInfo *titleInfo,
                                                        TitleSelectCommon *titleSelect);

        /// @brief Creates, pushes, and returns a new TitleOptionState
        static std::shared_ptr<TitleOptionState> create_and_push(data::User *user,
                                                                 data::TitleInfo *titleInfo,
                                                                 TitleSelectCommon *titleSelect);

        /// @brief Runs update routine.
        void update() override;

        /// @brief Runs the render routine.
        void render() override;

        /// @brief This function allows tasks to signal to the spawning state to close itself on the next update() call.
        void close_on_update();

        /// @brief Signals to the main thread that a view refresh is required on the next update() call.
        void refresh_required();

        // clang-format off
        struct DataStruct
        {
            data::User *user{};
            data::TitleInfo *titleInfo{};
            TitleOptionState *spawningState{};
            TitleSelectCommon *titleSelect{};
        };
        // clang-format on

        using TaskData = std::shared_ptr<TitleOptionState::DataStruct>;

    private:
        /// @brief This is just in case the option should only apply to the current user.
        data::User *m_user{};

        /// @brief This is the target title.
        data::TitleInfo *m_titleInfo{};

        /// @brief Pointer to the title selection being used for updating.
        TitleSelectCommon *m_titleSelect{};

        /// @brief The struct passed to functions.
        std::shared_ptr<TitleOptionState::DataStruct> m_dataStruct{};

        /// @brief This holds whether or not the state should deactivate itself on the next update loop.
        bool m_exitRequired{};

        /// @brief This stores whether or a not a refresh is required on the next update().
        bool m_refreshRequired{};

        /// @brief Menu used and shared by all instances.
        static inline std::shared_ptr<ui::Menu> sm_titleOptionMenu{};

        /// @brief This is shared by all instances of this class.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel{};

        void initialize_static_members();

        void initialize_data_struct();

        void create_push_info_state();

        void add_to_blacklist();

        void change_output_directory();

        void create_push_file_mode();

        void delete_all_local_backups();

        void delete_all_remote_backups();

        void reset_save_data();

        void delete_save_from_system();

        void extend_save_container();

        void export_svi_file();
};

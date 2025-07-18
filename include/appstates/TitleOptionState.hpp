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

        /// @brief Runs update routine.
        void update() override;

        /// @brief Runs the render routine.
        void render() override;

        /// @brief This function allows tasks to signal to the spawning state to close itself on the next update() call.
        void close_on_update();

        /// @brief Signals to the main thread that a view refresh is required on the next update() call.
        void refresh_required();

        /// @brief This is the struct used to pass data to the thread functions.
        typedef struct
        {
                /// @brief Pointer to the target user.
                data::User *m_user{};

                /// @brief The target title's data.
                data::TitleInfo *m_titleInfo{};

                /// @brief Allows tasks to signal deactivation.
                TitleOptionState *m_spawningState{};

                /// @brief The target title select. This is used for updating it.
                TitleSelectCommon *m_titleSelect{};
        } DataStruct;

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

        /// @brief This is so it's known whether or not to initialize the static members of this class.
        static inline bool sm_initialized{};

        /// @brief Menu used and shared by all instances.
        static inline std::unique_ptr<ui::Menu> sm_titleOptionMenu{};

        /// @brief This is shared by all instances of this class.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel{};
};

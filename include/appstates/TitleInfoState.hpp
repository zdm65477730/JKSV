#pragma once
#include "appstates/BaseState.hpp"
#include "data/data.hpp"
#include "sys/sys.hpp"
#include "ui/SlideOutPanel.hpp"
#include "ui/TextScroll.hpp"

#include <memory>
#include <string>
#include <vector>

class TitleInfoState final : public BaseState
{
    public:
        /// @brief Constructs a new title info state.
        /// @param user User to display info for.
        /// @param titleInfo Title to display info for.
        TitleInfoState(data::User *user, data::TitleInfo *titleInfo);

        /// @brief Required destructor.
        ~TitleInfoState();

        /// @brief Creates a new TitleInfoState.
        static std::shared_ptr<TitleInfoState> create(data::User *user, data::TitleInfo *titleInfo);

        /// @brief Creates, pushes, and returns a new TitleInfoState.
        static std::shared_ptr<TitleInfoState> create_and_push(data::User *user, data::TitleInfo *titleInfo);

        /// @brief Runs update routine.
        void update() override;

        /// @brief Runs render routine.
        void render() override;

    private:
        /// @brief Pointer to user.
        data::User *m_user{};

        /// @brief Pointer to title info.
        data::TitleInfo *m_titleInfo{};

        /// @brief This is a pointer to the title's icon.
        sdl::SharedTexture m_icon{};

        /// @brief This controls the clear color of the fields rendered.
        bool m_fieldClear{};

        /// @brief Slide panel.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel{};

        /// @brief Initializes the static members if they haven't been already.
        void initialize_static_members();

        /// @brief Creates the scrolling text/cheating fields.
        void create_info_scrolls();

        /// @brief Helper function for creating text fields.
        std::shared_ptr<ui::TextScroll> create_new_scroll(std::string_view text, int y);
};

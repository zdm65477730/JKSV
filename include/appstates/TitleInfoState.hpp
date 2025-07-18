#pragma once
#include "appstates/BaseState.hpp"
#include "data/data.hpp"
#include "system/Timer.hpp"
#include "ui/SlideOutPanel.hpp"
#include "ui/TextScroll.hpp"

#include <memory>
#include <string>

class TitleInfoState final : public BaseState
{
    public:
        /// @brief Constructs a new title info state.
        /// @param user User to display info for.
        /// @param titleInfo Title to display info for.
        TitleInfoState(data::User *user, data::TitleInfo *titleInfo);

        /// @brief Required destructor.
        ~TitleInfoState();

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

        /// @brief This is the scrolling text for the title.
        ui::TextScroll m_titleScroll{};

        /// @brief This is the scrolling text for the publisher.
        ui::TextScroll m_publisherScroll{};

        /// @brief This string holds the application ID.
        std::string m_applicationID{};

        /// @brief This holds the hex save data id of the file on nand.
        std::string m_saveDataID{};

        /// @brief This holds the time the game was first played.
        std::string m_firstPlayed{};

        /// @brief This holds the last played timestamp.
        std::string m_lastPlayed{};

        /// @brief This holds the play time string.
        std::string m_playTime{};

        /// @brief This holds the total launches string.
        std::string m_totalLaunches{};

        /// @brief This holds the save data type string.
        std::string m_saveDataType{};

        /// @brief Bool to tell whether or not static members are initialized.
        static inline bool sm_initialized{};

        /// @brief This is the render target for the title text just in case it needs scrolling.
        static inline sdl::SharedTexture sm_titleTarget{};

        /// @brief This is the render target for the publisher string.
        static inline sdl::SharedTexture sm_publisherTarget{};

        /// @brief Slide panel.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel{};
};

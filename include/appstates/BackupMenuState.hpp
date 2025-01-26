#pragma once
#include "appstates/AppState.hpp"
#include "data/data.hpp"
#include "fslib.hpp"
#include "sdl.hpp"
#include "system/Timer.hpp"
#include "ui/Menu.hpp"
#include "ui/SlideOutPanel.hpp"
#include <memory>

/// @brief This is the state where the user can backup and restore saves.
class BackupMenuState : public AppState
{
    public:
        /// @brief Creates a new backup selection state.
        /// @param user Pointer to currently selected user.
        /// @param titleInfo Pointer to titleInfo of selected title.
        /// @param saveType Save data type we're working with.
        BackupMenuState(data::User *user, data::TitleInfo *titleInfo, FsSaveDataType saveType);

        /// @brief Destructor. This is required even if it doesn't free or do anything.
        ~BackupMenuState() {};

        /// @brief Required. Inherited virtual function from AppState.
        void update(void);

        /// @brief Required. Inherited virtual function from AppState.
        void render(void);

        /// @brief Refreshes the directory listing and menu.
        void refresh(void);

    private:
        /// @brief Pointer to current user.
        data::User *m_user = nullptr;
        /// @brief Pointer to data for selected title.
        data::TitleInfo *m_titleInfo = nullptr;
        /// @brief Save data type we're working with.
        FsSaveDataType m_saveType;
        /// @brief Path to the target directory of the title.
        fslib::Path m_directoryPath;
        /// @brief Directory listing of the above.
        fslib::Directory m_directoryListing;
        /// @brief The width of the current title in pixels.
        int m_titleWidth = 0;
        /// @brief X coordinate to render the games's title at.
        int m_titleX = 0;
        /// @brief Whether or not the above is too long to be displayed at once and needs to be scrolled.
        bool m_titleScrolling = false;
        /// @brief Whether or not the scrolling timer was triggered and we should scroll the title string.
        bool m_titleScrollTriggered = false;
        /// @brief Timer for scrolling the title text if it's too long.
        sys::Timer m_titleScrollTimer;
        /// @brief Variable that saves whether or not the filesystem has data in it.
        bool m_saveHasData = false;

        /// @brief Whether or not anything beyond this point needs to be init'd. Everything here is static and shared by all instances.
        static inline bool sm_isInitialized = false;
        /// @brief The menu used by all instances of BackupMenuState.
        static inline std::unique_ptr<ui::Menu> m_backupMenu = nullptr;
        /// @brief The slide out panel used by all instances of BackupMenuState.
        static inline std::unique_ptr<ui::SlideOutPanel> m_slidePanel = nullptr;
        /// @brief Inner render target so the menu only renders to a certain area.
        static inline sdl::SharedTexture m_menuRenderTarget = nullptr;
        /// @brief The width of the panels. This is set according to the control guide text.
        static inline int m_panelWidth = 0;

        /// @brief Renders the title of the currently targetted game.
        void renderTitle(void);
};

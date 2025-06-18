#pragma once
#include "appstates/AppState.hpp"
#include "data/data.hpp"
#include "fslib.hpp"
#include "sdl.hpp"
#include "system/Timer.hpp"
#include "ui/Menu.hpp"
#include "ui/SlideOutPanel.hpp"
#include "ui/TextScroll.hpp"
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
        ~BackupMenuState();

        /// @brief Required. Inherited virtual function from AppState.
        void update(void) override;

        /// @brief Required. Inherited virtual function from AppState.
        void render(void) override;

        /// @brief Refreshes the directory listing and menu.
        void refresh(void);

        /// @brief Allows a spawned task to tell this class that it wrote save data to the system.
        void save_data_written(void);

        /// @brief Struct used for passing data to functions.
        typedef struct
        {
                /// @brief Pointer to the target user.
                data::User *m_user;

                /// @brief Data for the target title.
                data::TitleInfo *m_titleInfo;

                /// @brief Path of the target.
                fslib::Path m_targetPath;

                /// @brief Journal size for when a commit is needed.
                uint64_t m_journalSize;

                /// @brief Pointer to >this spawning state.
                BackupMenuState *m_spawningState;
        } DataStruct;

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

        /// @brief This is the scrolling text at the top.
        ui::TextScroll m_titleScroll;

        /// @brief Variable that saves whether or not the filesystem has data in it.
        bool m_saveHasData = false;

        /// @brief Data struct passed to functions.
        std::shared_ptr<BackupMenuState::DataStruct> m_dataStruct;

        /// @brief Whether or not anything beyond this point needs to be init'd. Everything here is static and shared by all instances.
        static inline bool sm_isInitialized = false;

        /// @brief The menu used by all instances of BackupMenuState.
        static inline std::shared_ptr<ui::Menu> sm_backupMenu = nullptr;

        /// @brief The slide out panel used by all instances of BackupMenuState.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel = nullptr;

        /// @brief Inner render target so the menu only renders to a certain area.
        static inline sdl::SharedTexture sm_menuRenderTarget = nullptr;

        /// @brief The width of the panels. This is set according to the control guide text.
        static inline int sm_panelWidth = 0;
};

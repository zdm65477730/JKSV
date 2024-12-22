#pragma once
#include "AppStates/AppState.hpp"
#include "Data/Data.hpp"
#include "FsLib.hpp"
#include "SDL.hpp"
#include "System/Timer.hpp"
#include "UI/Menu.hpp"
#include "UI/SlideOutPanel.hpp"
#include <memory>

class BackupMenuState : public AppState
{
    public:
        BackupMenuState(Data::User *User, Data::TitleInfo *TitleInfo);
        ~BackupMenuState() {};

        void Update(void);
        void Render(void);
        // Refreshes/Updates menu and listing.
        void RefreshListing(void);

    private:
        // Pointer to user.
        Data::User *m_User;
        // Pointer to title info.
        Data::TitleInfo *m_TitleInfo;
        // Backup folder path
        FsLib::Path m_DirectoryPath;
        // Directory listing of that folder.
        FsLib::Directory m_DirectoryListing;
        // Width of the title.
        int m_TitleWidth;
        // X coordinate of title.
        int m_TitleX;
        // Whether or not the title is too long for the panel and should scroll.
        bool m_ScrollTitle = false;
        // Whether or not the timer was triggered and we're scrolling.
        bool m_ScrollTriggered = false;
        // Timer for scrolling title if needed.
        System::Timer m_TitleTimer;
        // This holds whether or not the static members every instance shares are initialized.
        static inline bool m_Initialized = false;
        // Menu
        static inline std::unique_ptr<UI::Menu> m_BackupMenu = nullptr;
        // Slide panel.
        static inline std::unique_ptr<UI::SlideOutPanel> m_SlidePanel = nullptr;
        // Render target for menu.
        static inline SDL::SharedTexture m_MenuTarget = nullptr;
        // Width of panel.
        static inline int m_PanelWidth = 0;
        // X coordinate of text above menu.
        static inline int m_CurrentBackupsCoordinate = 0;
        // Function to render the title to the menu.
        void RenderTitle(void);
};

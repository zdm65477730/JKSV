#pragma once
#include "appstates/BaseState.hpp"
#include "data/data.hpp"
#include "fslib.hpp"
#include "sdl.hpp"
#include "system/Timer.hpp"
#include "ui/Menu.hpp"
#include "ui/SlideOutPanel.hpp"
#include "ui/TextScroll.hpp"

#include <memory>

/// @brief This is the state where the user can backup and restore saves.
class BackupMenuState final : public BaseState
{
    public:
        /// @brief Creates a new backup selection state.
        /// @param user Pointer to currently selected user.
        /// @param titleInfo Pointer to titleInfo of selected title.
        /// @param saveType Save data type we're working with.
        BackupMenuState(data::User *user, data::TitleInfo *titleInfo);

        /// @brief Destructor. This is required even if it doesn't free or do anything.
        ~BackupMenuState();

        /// @brief Required. Inherited virtual function from AppState.
        void update() override;

        /// @brief Required. Inherited virtual function from AppState.
        void render() override;

        /// @brief Refreshes the directory listing and menu.
        void refresh();

        /// @brief Allows a spawned task to tell this class that it wrote save data to the system.
        void save_data_written();

        // clang-format off
        enum class MenuEntryType
        {
            Null,
            Local,
            Remote
        };

        struct MenuEntry
        {
            MenuEntryType type;
            int index;
        };

        struct DataStruct
        {
            data::User *user{};
            data::TitleInfo *titleInfo{};
            fslib::Path path{};
            BackupMenuState *spawningState{};
        };
        // clang-format on

        // This makes some things elsewhere easier to type.
        using TaskData = std::shared_ptr<BackupMenuState::DataStruct>;

    private:
        /// @brief Pointer to current user.
        data::User *m_user{};

        /// @brief Pointer to data for selected title.
        data::TitleInfo *m_titleInfo{};

        /// @brief Save data type we're working with.
        FsSaveDataType m_saveType{};

        /// @brief Path to the target directory of the title.
        fslib::Path m_directoryPath{};

        /// @brief Directory listing of the above.
        fslib::Directory m_directoryListing{};

        /// @brief This is the scrolling text at the top.
        ui::TextScroll m_titleScroll{};

        /// @brief Variable that saves whether or not the filesystem has data in it.
        bool m_saveHasData{};

        /// @brief Data struct passed to functions.
        std::shared_ptr<BackupMenuState::DataStruct> m_dataStruct{};

        /// @brief This keeps track of the properties of the entries in the menu.
        std::vector<BackupMenuState::MenuEntry> m_menuEntries{};

        /// @brief This is a pointer to the control guide string.
        const char *m_controlGuide{};

        /// @brief The width of the panels. This is set according to the control guide text.
        static inline int sm_panelWidth{};

        /// @brief The menu used by all instances of BackupMenuState.
        static inline std::shared_ptr<ui::Menu> sm_backupMenu{};

        /// @brief The slide out panel used by all instances of BackupMenuState.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel{};

        /// @brief Inner render target so the menu only renders to a certain area.
        static inline sdl::SharedTexture sm_menuRenderTarget{};

        /// @brief Initializes the static members all instances share if they haven't been already.
        void initialize_static_members();

        /// @brief Checks for and tries to create the target directory if it hasn't been already.
        void ensure_target_directory();

        /// @brief Initializes the struct passed to tasks.
        void initialize_task_data();

        /// @brief Init's the string at the top of the backupmenu.
        void initialize_info_string();

        /// @brief Checks to see if the save data is empty.
        void save_data_check();

        /// @brief Ensures the remote storage is initalized and pointing to the right place.
        void initialize_remote_storage();

        /// @brief This is the function called when New Backup is selected.
        void name_and_create_backup();

        /// @brief This is the function called when a backup is selected to be overwritten.
        void confirm_overwrite();

        /// @brief This function is called to confirm restoring a backup.
        void confirm_restore();

        /// @brief Function called to confirm deleting a backup.
        void confirm_delete();

        /// @brief Uploads the currently selected backup to the remote storage.
        void upload_backup();

        /// @brief Just creates the pop-up that says Save is empty or w/e.
        void pop_save_empty();

        inline bool is_system_save_data()
        {
            return m_saveType == FsSaveDataType_System || m_saveType == FsSaveDataType_SystemBcat;
        }
};

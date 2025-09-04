#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "fslib.hpp"
#include "sdl.hpp"
#include "ui/ui.hpp"

#include <string_view>

class FileModeState final : public BaseState
{
    public:
        /// @brief Constructs a new FileModeState.
        FileModeState(std::string_view mountA, std::string_view mountB, int64_t journalSize = 0);

        /// @brief Destructor. Closes the filesystems passed.
        ~FileModeState() {};

        static inline std::shared_ptr<FileModeState> create(std::string_view mountA,
                                                            std::string_view mountB,
                                                            int64_t journalSize = 0)
        {
            return std::make_shared<FileModeState>(mountA, mountB, journalSize);
        }

        static inline std::shared_ptr<FileModeState> create_and_push(std::string_view mountA,
                                                                     std::string_view mountB,
                                                                     int64_t journalSize = 0)
        {
            auto newState = std::make_shared<FileModeState>(mountA, mountB, journalSize);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Update override.
        void update() override;

        /// @brief Render override.
        void render() override;

        /// @brief This thing is a headache without this.
        friend class FileOptionState;

    private:
        /// @brief These store the mount points to close the filesystems upon construction.
        std::string m_mountA;
        std::string m_mountB;

        /// @brief These are the actual target paths.
        fslib::Path m_pathA{};
        fslib::Path m_pathB{};

        /// @brief Directory listings for each respective path.
        fslib::Directory m_dirA{};
        fslib::Directory m_dirB{};

        /// @brief Menus for the directory listings.
        std::shared_ptr<ui::Menu> m_dirMenuA{};
        std::shared_ptr<ui::Menu> m_dirMenuB{};

        /// @brief Controls which menu/filesystem is currently targetted.
        bool m_target{};

        /// @brief Stores the size for committing data (if needed) to mountA.
        int64_t m_journalSize{};

        /// @brief The beginning Y coord of the dialog.
        double m_y{720.0f};

        /// @brief This is the targetY. Used for the opening and hiding effect.
        double m_targetY{91.0f};

        /// @brief Config scaling for the "transition"
        double m_scaling{};

        /// @brief Stores whether or not the dialog has reached its targetted position.
        bool m_inPlace{};

        /// @brief Frame shared by all instances.
        static inline std::shared_ptr<ui::Frame> sm_frame{};

        /// @brief This is the render target the browsers are rendered to.
        static inline sdl::SharedTexture sm_renderTarget{};

        /// @brief Stores a pointer to the guide in the bottom-right corner.
        static inline const char *sm_controlGuide{};

        /// @brief Calculated X coordinate of the control guide text.
        static inline int sm_controlGuideX{};

        /// @brief Initializes the members shared by all instances of FileModeState.
        void initialize_static_members();

        /// @brief Creates valid paths with the mounts passed.
        void initialize_paths();

        /// @brief Ensures the menus are allocated and setup properly.
        void initialize_menus();

        /// @brief Loads the current directory listings and menus.
        void initialize_directory_menu(const fslib::Path &path, fslib::Directory &directory, ui::Menu &menu);

        /// @brief Updates the dialog's coordinates in the beginning.
        void update_y_coord() noexcept;

        /// @brief Starts the dialog hiding process.
        void hide_dialog() noexcept;

        /// @brief Returns whether or not the dialog is hidden.
        bool is_hidden() noexcept;

        /// @brief Handles changing the current directory or opening the options.
        void enter_selected(fslib::Path &path, fslib::Directory &directory, ui::Menu &menu);

        /// @brief Opens the little option pop-up thingy.
        void open_option_menu(fslib::Directory &directory, ui::Menu &menu);

        /// @brief Changes the current target/controllable menu.
        void change_target() noexcept;

        /// @brief Function executed when '..' is selected.
        void up_one_directory(fslib::Path &path, fslib::Directory &directory, ui::Menu &menu);

        /// @brief Appends the entry passed to the path passed and "enters" the directory.
        /// @param path Working path.
        /// @param directory Working directory.
        /// @param menu Working menu.
        /// @param entry Target entry.
        void enter_directory(fslib::Path &path,
                             fslib::Directory &directory,
                             ui::Menu &menu,
                             const fslib::DirectoryEntry &entry);

        /// @brief Renders the control guide string on the bottom of the screen.
        void render_control_guide();

        /// @brief Returns a reference to the currently active menu.
        ui::Menu &get_source_menu() noexcept;

        /// @brief Returns a reference to the currently inactive menu.
        ui::Menu &get_destination_menu() noexcept;

        /// @brief Returns a reference to the current "active" path.
        fslib::Path &get_source_path() noexcept;

        /// @brief Returns a reference to the currently "inactive" path.
        fslib::Path &get_destination_path() noexcept;

        /// @brief Returns a reference to the "active" directory.
        fslib::Directory &get_source_directory() noexcept;

        /// @brief Returns a reference ot the currently "inactive" directory.
        fslib::Directory &get_destination_directory() noexcept;

        /// @brief Returns whether or not a path is at the root.
        inline bool path_is_root(fslib::Path &path) const { return std::char_traits<char>::length(path.get_path()) <= 1; }

        /// @brief Closes the filesystems passed and deactivates the state.
        void deactivate_state() noexcept;
};
#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "appstates/FileModeState.hpp"
#include "fslib.hpp"
#include "ui/ui.hpp"

class FileOptionState final : public BaseState
{
    public:
        /// @brief FileOptionState.
        /// @param spawningState Pointer to spawning state to grab its goodies.
        FileOptionState(FileModeState *spawningState);

        ~FileOptionState() {};

        /// @brief Inline creation function.
        static inline std::shared_ptr<FileOptionState> create(FileModeState *spawningState)
        {
            return std::make_shared<FileOptionState>(spawningState);
        }

        /// @brief Same as above. Pushes state before returning it.
        static inline std::shared_ptr<FileOptionState> create_and_push(FileModeState *spawningState)
        {
            auto newState = FileOptionState::create(spawningState);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Update routine.
        void update() override;

        /// @brief Render routine.
        void render() override;

        // clang-format off
        struct DataStruct
        {
            fslib::Path sourcePath{};
            fslib::Path destPath{};
            int64_t journalSize{};
        };
        // clang-format on

        /// @brief This makes some other stuff easier to read and type.
        using TaskData = std::shared_ptr<FileOptionState::DataStruct>;

    private:
        /// @brief Pointer to spawning FileMode state.
        FileModeState *m_spawningState{};

        /// @brief Stores whether or not tasks require committing data and changes to the target.
        bool m_commitData{};

        /// @brief Journal size for when committing is required.
        int64_t m_journalSize{};

        /// @brief X coordinate. This is set at construction according to the target from the spawning state.
        int m_x{};

        /// @brief X coordinate for the target to reach.
        int m_targetX{};

        /// @brief Whether or not the dialog/menu is in place.
        bool m_inPlace{};

        /// @brief Whether or not the state should be closed.
        bool m_close{};

        /// @brief This holds the scaling in config.
        double m_scaling{};

        /// @brief This is the data struct passed to tasks.
        std::shared_ptr<FileOptionState::DataStruct> m_dataStruct{};

        /// @brief This is shared by all instances.
        static inline std::shared_ptr<ui::Menu> sm_copyMenu{};

        /// @brief This is shared by all instances.
        static inline std::shared_ptr<ui::DialogBox> sm_dialog{};

        /// @brief Ensures static members of all instances are allocated.
        void initialize_static_members();

        /// @brief Sets whether the dialog/menu are positioned left or right depending on the menu active in the spawning state.
        void set_menu_side();

        /// @brief Updates the Y coordinate
        void update_x_coord();

        void copy_target();

        void delete_target();

        void rename_target();

        void create_directory();

        void get_show_target_properties();

        /// @brief Closes and hides the state.
        void close();

        /// @brief Returns whether or not the state is closed.
        bool is_closed();

        /// @brief Sets the menu index back to 0 and deactivates the state.
        void deactivate_state();
};
#pragma once
#include "StateManager.hpp"
#include "appstates/BaseTask.hpp"
#include "sdl.hpp"

#include <functional>
#include <vector>

class DataLoadingState final : public BaseTask
{
    public:
        /// @brief This is a definition for functions that are called at destruction.
        using DestructFunction = std::function<void()>;

        template <typename... Args>
        DataLoadingState(void (*function)(sys::Task *, Args...), Args... args)
            : BaseTask()
        {
            DataLoadingState::initialize_static_members();
            m_task = std::make_unique<sys::Task>(function, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static std::shared_ptr<DataLoadingState> create(void (*function)(sys::Task *, Args...), Args... args)
        {
            return std::make_shared<DataLoadingState>(function, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static std::shared_ptr<DataLoadingState> create_and_push(void (*function)(sys::Task *, Args...), Args... args)
        {
            auto newState = DataLoadingState::create(function, std::forward<Args>(args)...);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Destructor. Runs all DestructFunctions in the vector.
        ~DataLoadingState();

        /// @brief Update override.
        void update() override;

        /// @brief Render override.
        void render() override;

        /// @brief Executes the destruct functions passed.
        void execute_destructs();

        /// @brief Adds a function to the vector to be executed upon destruction.
        void add_destruct_function(DataLoadingState::DestructFunction function);

    private:
        /// @brief X coord of the status text.
        int m_statusX{};

        /// @brief The functions called upon destruction.
        std::vector<DestructFunction> m_destructFunctions{};

        /// @brief Icon displayed in the center of the screen.
        static inline sdl::SharedTexture sm_jksvIcon{};

        /// @brief Loads the icon if it hasn't been already.
        void initialize_static_members();
};

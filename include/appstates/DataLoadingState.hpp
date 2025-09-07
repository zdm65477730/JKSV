#pragma once
#include "StateManager.hpp"
#include "appstates/BaseTask.hpp"
#include "data/DataContext.hpp"
#include "sdl.hpp"
#include "sys/OpTimer.hpp"

#include <functional>
#include <vector>

class DataLoadingState final : public BaseTask
{
    public:
        /// @brief This is a definition for functions that are called at destruction.
        using DestructFunction = std::function<void()>;

        template <typename... Args>
        DataLoadingState(data::DataContext &context,
                         DestructFunction destructFunction,
                         void (*function)(sys::Task *, Args...),
                         Args... args)
            : BaseTask()
            , m_context(context)
            , m_destructFunction(destructFunction)
        {
            DataLoadingState::initialize_static_members();
            m_task = std::make_unique<sys::Task>(function, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static inline std::shared_ptr<DataLoadingState> create(data::DataContext &context,
                                                               DestructFunction destructFunction,
                                                               void (*function)(sys::Task *, Args...),
                                                               Args... args)
        {
            return std::make_shared<DataLoadingState>(context, destructFunction, function, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static inline std::shared_ptr<DataLoadingState> create_and_push(data::DataContext &context,
                                                                        DestructFunction destructFunction,
                                                                        void (*function)(sys::Task *, Args...),
                                                                        Args... args)
        {
            auto newState = DataLoadingState::create(context, destructFunction, function, std::forward<Args>(args)...);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Update override.
        void update() override;

        /// @brief Render override.
        void render() override;

    private:
        /// @brief Reference to the data context to run post-init operations.
        data::DataContext &m_context;

        /// @brief X coord of the status text.
        int m_statusX{};

        /// @brief The functions called upon destruction.
        DestructFunction m_destructFunction{};

        /// @brief Icon displayed in the center of the screen.
        static inline sdl::SharedTexture sm_jksvIcon{};

        /// @brief Loads the icon if it hasn't been already.
        void initialize_static_members();

        /// @brief
        void deactivate_state();
};

#pragma once
#include "appstates/BaseTask.hpp"

#include <functional>

/// @brief This is basically a task state that allows a callback and custom render function to be set.
class TaskCallbackState final : public BaseTask
{
    public:
        /// @brief
        using CallbackFunction = std::function<void(sys::Task *)>;

        template <typename... Args>
        TaskCallbackState(void (*taskFunction)(sys::Task *, Args...),
                          CallbackFunction callbackFunction,
                          CallbackFunction renderFunction,
                          Args... args)
            : m_callbackFunction(callbackFunction)
            , m_renderFunction(renderFunction)
        {
            m_task = std::make_unique<sys::Task>(taskFunction, std::forward<Args>(args)...);
        }

        /// @brief Update override. Calls m_callbackFunction.
        void update() override;

        /// @brief Render override. Calls m_renderFunction.
        void render() override;

    private:
        /// @brief Callback that is called on update.
        CallbackFunction m_callbackFunction{};

        /// @brief Callback that is called on render.
        CallbackFunction m_renderFunction{};
};

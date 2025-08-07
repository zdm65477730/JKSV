#pragma once
#include "appstates/BaseState.hpp"
#include "ui/DialogBox.hpp"

#include <memory>
#include <string>

class MessageState final : public BaseState
{
    public:
        /// @brief Constructor.
        /// @param message Message to display.
        MessageState(std::string_view message);

        /// @brief Required. Does NOTHING~
        ~MessageState();

        /// @brief Creates and returns a new MessageState. See constructor.
        static std::shared_ptr<MessageState> create(std::string_view message);

        /// @brief Same as above, only pushed to the StateManager before return.
        static std::shared_ptr<MessageState> create_and_push(std::string_view message);

        /// @brief Same as above, but creates and pushes a transition fade in between.
        static std::shared_ptr<MessageState> create_and_push_fade(std::string_view message);

        /// @brief Update override.
        void update() override;

        /// @brief Render override
        void render() override;

    private:
        /// @brief Safe store of the message to display.
        std::string m_message{};

        /// @brief Prevents the state from auto triggering from a previous frame.
        bool m_triggerGuard{};

        /// @brief This is the OK string.
        static inline const char *sm_okText{};

        /// @brief The center X coord for the OK [A]
        static inline int sm_okX{};

        /// @brief All instances share this. There's no point in constantly allocating a new one.
        static inline std::shared_ptr<ui::DialogBox> sm_dialog{};

        /// @brief Allocates and ensures ^
        void initialize_static_members();
};

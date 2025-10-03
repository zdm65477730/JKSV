#pragma once
#include "sys/Timer.hpp"
#include "ui/DialogBox.hpp"
#include "ui/Transition.hpp"

#include <string>

namespace ui
{
    class PopMessage
    {
        public:
            /// @brief PopMessage constructor.
            PopMessage(int ticks, std::string_view message);

            /// @brief Same as above. Moves string instead of copying.
            PopMessage(int ticks, std::string &message);

            /// @brief Updates the width, target, and typing.
            void update(double targetY);

            /// @brief Renders the dialog and text.
            void render();

            /// @brief Returns whether or not the message can be purged.
            bool finished() const noexcept;

            /// @brief Returns the text of the message.
            std::string_view get_message() const noexcept;

        private:
            /// @brief Transition for the pop up/pop out thing.
            ui::Transition m_transition{};

            /// @brief Ticks to start timer with.
            int m_ticks{};

            /// @brief This stores the message for safe keeping.
            std::string m_message{};

            /// @brief Current rendering coordinate for the text.
            int m_textX{};

            /// @brief Whether or not the targetY coordinate was met.
            bool m_yMet{};

            /// @brief Whether
            bool m_close{};

            /// @brief Returns whether or not the message has reached the end of its life.
            bool m_finished{};

            /// @brief The current offset of the substr.
            int m_substrOffset{};

            /// @brief Times when the message should be nuked.
            sys::Timer m_displayTimer{};

            /// @brief This times  updates the end of the substr printed.
            sys::Timer m_typeTimer{};

            /// @brief Dialog used to render the background.
            std::shared_ptr<ui::DialogBox> m_dialog{};

            /// @brief Updates the Y Coord to match the target passed.
            void update_y(double targetY) noexcept;

            /// @brief Updates the current end offset of the text.
            void update_text_offset();

            /// @brief Updates the width of the dialog.
            void update_width() noexcept;

            /// @brief Signals the pop message to close.
            void close() noexcept;
    };
}

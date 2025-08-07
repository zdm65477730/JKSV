#pragma once
#include "sys/Timer.hpp"
#include "ui/DialogBox.hpp"

#include <string>

namespace ui
{
    class PopMessage
    {
        public:
            /// @brief PopMessage constructor.
            PopMessage(int ticks, std::string_view message);

            /// @brief Updates the width, target, and typing.
            void update(double targetY);

            /// @brief Renders the dialog and text.
            void render();

            /// @brief Returns whether or not the message can be purged.
            bool finished() const;

        private:
            // Every message begins off screen.
            static inline constexpr int START_X      = 624;
            static inline constexpr double START_Y   = 720;
            static inline constexpr int START_WIDTH  = 32;
            static inline constexpr int PERMA_HEIGHT = 48;

            /// @brief Ticks to start timer with.
            int m_ticks{};

            /// @brief This stores the message for safe keeping.
            std::string m_message{};

            /// @brief The current, actual Y coord.
            double m_y{};

            /// @brief Currently rendering coordinate for the text.
            int m_textX{};

            /// @brief Current width;
            int m_width{};

            /// @brief Whether or not the targetY coordinate was met.
            bool m_yMet{};

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
            void update_y(double targetY);

            /// @brief Updates the current end offset of the text.
            void update_text_offset();
    };
}

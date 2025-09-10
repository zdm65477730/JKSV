#pragma once

namespace ui
{
    class Transition final
    {
        public:
            /// @brief Default.
            Transition() = default;

            /// @brief Constructs a new transition.
            /// @param x Beginning X coord.
            /// @param y Beginning Y coord.
            /// @param targetX Target X coord.
            /// @param targetY Target Y coord.
            /// @param threshold Gap, in pixels, before the x and/or y clamp into position.
            Transition(int x, int y, int targetX, int targetY, int threshold) noexcept;

            /// @brief Updates the transition.
            void update() noexcept;

            /// @brief Returns whether or not the transition has been met.
            bool in_place() const noexcept;

            /// @brief Returns the X coordinate.
            int get_x() const noexcept;

            /// @brief Returns the Y coordinate.
            int get_y() const noexcept;

            /// @brief Returns the target X.
            int get_target_x() const noexcept;

            /// @brief Returns the target Y.
            int get_target_y() const noexcept;

            /// @brief Sets the X coordinate.
            void set_x(int x) noexcept;

            /// @brief Sets the Y coordinate.
            void set_y(int y) noexcept;

            /// @brief Sets the target X coord.
            void set_target_x(int targetX) noexcept;

            /// @brief Sets the target Y coord.
            void set_target_y(int targetY) noexcept;

        private:
            /// @brief Current X.
            double m_x{};

            /// @brief Current Y.
            double m_y{};

            /// @brief Target X.
            double m_targetX{};

            /// @brief Target Y.
            double m_targetY{};

            /// @brief Pixel gap threshold.
            double m_threshold{};

            /// @brief Scaling from config.
            double m_scaling{};

            void update_x_coord() noexcept;

            void update_y_coord() noexcept;
    };
}
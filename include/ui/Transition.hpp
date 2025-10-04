#pragma once

namespace ui
{
    class Transition final
    {
        public:
            /// @brief Default.
            Transition();

            /// @brief Constructs a new transition.
            /// @param x Beginning X coord.
            /// @param y Beginning Y coord.
            /// @param targetX Target X coord.
            /// @param targetY Target Y coord.
            /// @param threshold Gap, in pixels, before the x and/or y clamp into position.
            Transition(int x,
                       int y,
                       int width,
                       int height,
                       int targetX,
                       int targetY,
                       int targetWidth,
                       int targetHeight,
                       int threshold) noexcept;

            /// @brief Updates the transition.
            void update() noexcept;

            /// @brief Updates only the X and Y.
            void update_xy() noexcept;

            /// @brief Updates only the width and height.
            void update_width_height() noexcept;

            /// @brief Returns whether or not the transition has been met.
            bool in_place() const noexcept;

            /// @brief Returns if the target X and Y coordinates were met.
            bool in_place_xy() const noexcept;

            /// @brief Returns if the target width and height were met.
            bool in_place_width_height() const noexcept;

            /// @brief Returns the X coordinate.
            int get_x() const noexcept;

            /// @brief Returns the Y coordinate.
            int get_y() const noexcept;

            /// @brief Returns the current width.
            int get_width() const noexcept;

            /// @brief Returns the current height.
            int get_height() const noexcept;

            /// @brief Returns the target X.
            int get_target_x() const noexcept;

            /// @brief Returns the target Y.
            int get_target_y() const noexcept;

            /// @brief Returns the target width.
            int get_target_width() const noexcept;

            /// @brief Returns the target height.
            int get_target_height() const noexcept;

            /// @brief Returns a screen centered coordinate according to the current x.
            int get_centered_x() const noexcept;

            /// @brief Returns a screen centered coordinate according to the current y.
            int get_centered_y() const noexcept;

            /// @brief Sets the X coordinate.
            void set_x(int x) noexcept;

            /// @brief Sets the Y coordinate.
            void set_y(int y) noexcept;

            /// @brief Sets the current width.
            void set_width(int width) noexcept;

            /// @brief Sets the current height.
            void set_height(int height) noexcept;

            /// @brief Sets the target X coord.
            void set_target_x(int targetX) noexcept;

            /// @brief Sets the target Y coord.
            void set_target_y(int targetY) noexcept;

            /// @brief Sets the target width of the transition.
            void set_target_width(int targetWidth) noexcept;

            /// @brief Sets the target height of the transition.
            void set_target_height(int targetHeight) noexcept;

            /// @brief Sets the threshold before snapping occurs.
            void set_threshold(int threshold) noexcept;

            static inline int DEFAULT_THRESHOLD = 2;

        private:
            /// @brief Current X.
            double m_x{};

            /// @brief Current Y.
            double m_y{};

            /// @brief Current width.
            double m_width{};

            /// @brief Current height.
            double m_height{};

            /// @brief Target X.
            double m_targetX{};

            /// @brief Target Y.
            double m_targetY{};

            /// @brief Target/end width.
            double m_targetWidth{};

            /// @brief Target/end height.
            double m_targetHeight{};

            /// @brief Pixel gap threshold.
            double m_threshold{};

            /// @brief Scaling from config.
            double m_scaling{};

            void update_x_coord() noexcept;

            void update_y_coord() noexcept;

            void update_width() noexcept;

            void update_height() noexcept;
    };
}

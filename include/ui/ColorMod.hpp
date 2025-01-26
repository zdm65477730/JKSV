#pragma once
#include <cstdint>

namespace ui
{
    /// @brief This class updates and keeps track of a color modifying variable for ui elements that need one.
    class ColorMod
    {
        public:
            /// @brief Default constructor.
            ColorMod(void) = default;

            /// @brief Updates the color modification variable.
            void update(void);

            /// @brief Allows me to use this like it's a uint8_t directly.
            operator uint8_t(void) const;

        private:
            /// @brief Whether we're adding or subtracting from the color value.
            bool m_direction = true;
            /// @brief Color value.
            uint8_t m_colorMod = 0;
    };
} // namespace ui

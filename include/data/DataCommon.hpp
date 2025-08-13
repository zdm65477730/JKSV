#pragma once
#include "sdl.hpp"

namespace data
{
    class DataCommon
    {
        public:
            /// @brief Default
            DataCommon() = default;

            /// @brief Function to load the icon to a texture.
            virtual void load_icon() = 0;

            /// @brief Returns the icon.
            sdl::SharedTexture get_icon() { return m_icon; };

            /// @brief Sets the icon.
            void set_icon(sdl::SharedTexture &icon) { m_icon = icon; };

        protected:
            /// @brief Shared texture of the icon.
            sdl::SharedTexture m_icon{};
    };
}

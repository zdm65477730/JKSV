#pragma once
#include "sdl.hpp"

namespace ui
{
    /// @brief Base class for ui elements.
    class Element
    {
        public:
            /// @brief Default constructor.
            Element() = default;

            /// @brief Virtual destructor.
            virtual ~Element() {};

            /// @brief Virtual update method. All derived classes must have this.
            /// @param HasFocus Whether or not the state containing the element currently has focus.
            virtual void update(bool HasFocus) = 0;

            /// @brief Virtual render method. All derived classes must have this.
            /// @param target Target to render to.
            /// @param hasFocus Whether or not the containing state has focus.
            virtual void render(sdl::SharedTexture &target, bool hasFocus) = 0;
    };
} // namespace ui

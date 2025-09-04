#pragma once

namespace math
{
    template <typename Type>
    class Util
    {
        public:
            static inline Type absolute_distance(Type a, Type b) noexcept { return a > b ? a - b : b - a; }
    };
}

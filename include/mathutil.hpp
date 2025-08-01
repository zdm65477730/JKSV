#pragma once

namespace math
{
    template <typename Type>
    class Util
    {
        public:
            static inline Type get_absolute_distance(Type a, Type b) { return a > b ? a - b : b - a; }
    };
}

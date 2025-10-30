#pragma once

namespace math
{
    template <typename Type>
        requires std::integral<Type> || std::floating_point<Type>
    class Util
    {
        public:
            static inline Type absolute_distance(Type a, Type b) noexcept { return a > b ? a - b : b - a; }
    };

}

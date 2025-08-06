#pragma once
#include <string>
#include <vector>

namespace fs
{
    class PathFilter
    {
        public:
            
            PathFilter(std::string_view filterPath);

            bool is_filtered(std::string_view path);

        private:
            std::vector<std::string> m_paths{};
    };
}

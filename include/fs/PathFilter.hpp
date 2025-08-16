#pragma once
#include "fslib.hpp"

#include <string>
#include <vector>

namespace fs
{
    class PathFilter
    {
        public:
            /// @brief Loads a path filter JSON file.
            PathFilter(const fslib::Path &filterPath);

            /// @brief Returns whether or not the filter has valid paths.
            bool has_paths() const;

            /// @brief Returns whether or not the path passed is filtered.
            bool is_filtered(const fslib::Path &path);

        private:
            /// @brief Vector of paths to filter from deletion and backup.
            std::vector<std::string> m_paths{};
    };
}

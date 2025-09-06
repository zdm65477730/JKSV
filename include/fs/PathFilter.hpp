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
            bool has_paths() const noexcept;

            /// @brief Returns whether or not the path passed is filtered.
            bool is_filtered(const fslib::Path &path) const noexcept;

        private:
            /// @brief Vector of paths to filter from deletion and backup.
            std::vector<fslib::Path> m_paths{};
    };
}

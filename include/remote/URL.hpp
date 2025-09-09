#pragma once
#include <string>

namespace remote
{
    /// @brief This is a class to make URLs easier to build for Google Drive and WebDav.
    /// @note Normally I don't go this route, but it makes things easier.
    class URL final
    {
        public:
            /// @brief Default
            URL() = default;

            /// @brief Constructs a URL with a base URL already in place.
            /// @param base String_view containing the base URL.
            URL(std::string_view base);

            /// @brief Appends the string passed as a path to the URL
            /// @param path Path to append;
            URL &append_path(std::string_view path);

            /// @brief Appends a string parameter
            /// @param param Parameter to append.
            /// @param value Value of the parameter to append.
            URL &append_parameter(std::string_view param, std::string_view value);

            /// @brief Appends a trailing slash if needed.
            URL &append_slash();

            /// @brief Returns the C string of the url string.
            const char *get() const noexcept;

        private:
            static inline constexpr int SIZE_URL_BUFFER = 0x800;

            /// @brief This is where the actual URL is held.
            char m_urlBuffer[SIZE_URL_BUFFER] = {0};

            // Current offset in the buffer.
            int m_offset{};
    };
} // namespace remote

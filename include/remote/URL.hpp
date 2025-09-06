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

            /// @brief Copy constructor.
            /// @param url URL to copy.
            URL(const URL &url);

            /// @brief Move constructor.
            /// @param url URL to move.
            URL(URL &&url) noexcept;

            /// @brief Makes a copy of the URL passed.
            /// @param url remote::URL instance to make a copy of.
            URL &operator=(const URL &url);

            /// @brief Move operator.
            /// @param url URL to move.
            URL &operator=(URL &&url) noexcept;

            /// @brief Sets the base URL. Basically resets the string back to square 0.
            /// @param base Base URL to start with.
            URL &set_base(std::string_view base);

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
            /// @brief This is where the actual URL is held.
            std::string m_url{};

            /// @brief This checks and appends the necessary separator to the URL string.
            void append_separator();
    };
} // namespace remote

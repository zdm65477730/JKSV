#pragma once
#include <string>

namespace remote
{
    /// @brief This is a class to build URL encoded form bodies and be more readable than snprintfs.
    class Form
    {
        public:
            Form() = default;

            /// @brief Appends a parameter to the form/URL encoded text.
            /// @param param Parameter to append.
            /// @param value Value to append.
            Form &append_parameter(std::string_view param, std::string_view value) noexcept;

            /// @brief Returns m_form.length()
            size_t length() const noexcept;

            /// @brief Returns m_formBuffer;
            /// @return
            const char *get() const noexcept;

        private:
            /// @brief Size used for the buffer.
            static inline constexpr size_t SIZE_FORM_BUFFER = 0x800;

            /// @brief Current offset if the form.
            size_t m_offset{};

            /// @brief Buffer for the form.
            char m_formBuffer[SIZE_FORM_BUFFER] = {0};
    };
} // namespace remote

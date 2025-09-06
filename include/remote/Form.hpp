#pragma once
#include <string>

namespace remote
{
    /// @brief This is a class to build URL encoded form bodies and be more readable than snprintfs.
    class Form
    {
        public:
            Form() = default;

            /// @brief Copy constructor
            /// @param form Form to copy from.
            Form(const Form &form);

            /// @brief Move constructor.
            /// @param form Form to copy from.
            Form(Form &&form) noexcept;

            /// @brief = Operator.
            /// @param form Form to copy.
            Form &operator=(const Form &form) noexcept;

            /// @brief = Move operator.
            /// @param form Form to rob of its life.
            Form &operator=(Form &&form);

            /// @brief Appends a parameter to the form/URL encoded text.
            /// @param param Parameter to append.
            /// @param value Value to append.
            Form &append_parameter(std::string_view param, std::string_view value);

            /// @brief Returns the C string of the form string.
            const char *get() const noexcept;

            /// @brief Returns m_form.length()
            size_t length() const noexcept;

        private:
            /// @brief String containing the actual data posted.
            std::string m_form{};
    };
} // namespace remote

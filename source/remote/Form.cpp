#include "remote/Form.hpp"

#include <algorithm>
#include <cstring>
#include <span>

//                      ---- Public functions ----

remote::Form &remote::Form::append_parameter(std::string_view param, std::string_view value) noexcept
{
    const size_t paramLength = param.length();
    const size_t valueLength = value.length();
    const size_t endLength   = m_offset + paramLength + valueLength + 1;
    if (endLength >= SIZE_FORM_BUFFER) { return *this; }

    if (m_offset > 0 && m_formBuffer[m_offset - 1] != '&') { m_formBuffer[m_offset++] = '&'; }

    // Append the parameter first.
    size_t remainingLength = SIZE_FORM_BUFFER - m_offset;
    std::span<char> bufferSpan{&m_formBuffer[m_offset], remainingLength};
    std::copy(param.begin(), param.end(), bufferSpan.begin());
    m_offset += paramLength;

    // Add the =
    m_formBuffer[m_offset++] = '=';

    // Recalc, append value.
    remainingLength = SIZE_FORM_BUFFER - m_offset;
    bufferSpan      = std::span<char>{&m_formBuffer[m_offset], remainingLength};
    std::copy(value.begin(), value.end(), bufferSpan.begin());
    m_offset += valueLength;

    return *this;
}

const char *remote::Form::get() const noexcept { return m_formBuffer; }

size_t remote::Form::length() const noexcept { return m_offset; }

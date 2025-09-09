#include "remote/Form.hpp"

#include <algorithm>
#include <cstring>

remote::Form &remote::Form::append_parameter(std::string_view param, std::string_view value) noexcept
{
    const size_t paramLength = param.length();
    const size_t valueLength = value.length();
    const size_t endLength   = m_offset + paramLength + valueLength + 1;
    if (endLength >= SIZE_FORM_BUFFER) { return *this; }

    if (m_offset > 0 && m_formBuffer[m_offset] != '&') { m_formBuffer[m_offset++] = '&'; }

    std::copy(param.begin(), param.end(), &m_formBuffer[m_offset]);
    m_offset += paramLength;

    m_formBuffer[m_offset++] = '=';

    std::copy(value.begin(), value.end(), &m_formBuffer[m_offset]);
    m_offset += valueLength;

    return *this;
}

const char *remote::Form::get() const noexcept { return m_formBuffer; }

size_t remote::Form::length() const noexcept { return m_offset; }

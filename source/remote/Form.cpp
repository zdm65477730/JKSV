#include "remote/Form.hpp"

#include <cstring>

remote::Form &remote::Form::append_parameter(std::string_view param, std::string_view value)
{
    if (m_offset > 0 && m_formBuffer[m_offset] != '&') { m_formBuffer[m_offset++] = '&'; }

    const size_t paramLength = param.length();
    std::memcpy(&m_formBuffer[m_offset], param.data(), paramLength);
    m_offset += paramLength;

    m_formBuffer[m_offset++] = '=';

    const size_t valueLength = value.length();
    std::memcpy(&m_formBuffer[m_offset], value.data(), valueLength);
    m_offset += valueLength;

    return *this;
}

const char *remote::Form::get() const noexcept { return m_formBuffer; }

size_t remote::Form::length() const noexcept { return m_offset; }

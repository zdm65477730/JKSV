#include "remote/Form.hpp"

#include <algorithm>
#include <cstring>

remote::Form &remote::Form::append_parameter(std::string_view param, std::string_view value)
{
    if (m_offset > 0 && m_formBuffer[m_offset] != '&') { m_formBuffer[m_offset++] = '&'; }

    std::copy(param.begin(), param.end(), &m_formBuffer[m_offset]);
    m_offset += param.length();

    m_formBuffer[m_offset++] = '=';

    std::copy(value.begin(), value.end(), &m_formBuffer[m_offset]);
    m_offset += value.length();

    return *this;
}

const char *remote::Form::get() const noexcept { return m_formBuffer; }

size_t remote::Form::length() const noexcept { return m_offset; }

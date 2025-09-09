#include "remote/URL.hpp"

#include <algorithm>

remote::URL::URL(std::string_view base)
{
    std::copy(base.begin(), base.end(), &m_urlBuffer[m_offset]);
    m_offset += base.length();
}

remote::URL &remote::URL::append_path(std::string_view path)
{
    if (path.empty()) { return *this; }

    if (m_urlBuffer[m_offset] != '/' && path.front() != '/') { m_urlBuffer[m_offset++] = '/'; }

    std::copy(path.begin(), path.end(), &m_urlBuffer[m_offset]);
    m_offset += path.length();

    return *this;
}

remote::URL &remote::URL::append_parameter(std::string_view param, std::string_view value)
{
    const char *find = std::char_traits<char>::find(m_urlBuffer, m_offset, '?');
    if (!find) { m_urlBuffer[m_offset++] = '?'; }
    else { m_urlBuffer[m_offset++] = '&'; }

    std::copy(param.begin(), param.end(), &m_urlBuffer[m_offset]);
    m_offset += param.length();

    m_urlBuffer[m_offset++] = '=';

    std::copy(value.begin(), value.end(), &m_urlBuffer[m_offset]);
    m_offset += value.length();

    return *this;
}

remote::URL &remote::URL::append_slash()
{
    if (m_urlBuffer[m_offset] != '/') { m_urlBuffer[m_offset++] = '/'; }

    return *this;
}

const char *remote::URL::get() const noexcept { return m_urlBuffer; }

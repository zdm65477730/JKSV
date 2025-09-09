#include "remote/URL.hpp"

#include <algorithm>

remote::URL::URL(std::string_view base) noexcept
{
    const size_t baseLength = base.length();
    if (baseLength >= SIZE_URL_BUFFER) { return; }

    std::copy(base.begin(), base.end(), &m_urlBuffer[m_offset]);
    m_offset += base.length();
}

remote::URL &remote::URL::append_path(std::string_view path) noexcept
{
    if (path.empty()) { return *this; }

    const size_t pathLength = path.length();
    if (m_offset + pathLength >= SIZE_URL_BUFFER) { return *this; }

    if (m_urlBuffer[m_offset] != '/' && path.front() != '/') { m_urlBuffer[m_offset++] = '/'; }

    std::copy(path.begin(), path.end(), &m_urlBuffer[m_offset]);
    m_offset += pathLength;

    return *this;
}

remote::URL &remote::URL::append_parameter(std::string_view param, std::string_view value) noexcept
{
    const size_t paramLength = param.length();
    const size_t valueLength = value.length();
    const size_t endLength   = m_offset + paramLength + valueLength + 2;
    if (endLength >= SIZE_URL_BUFFER) { return *this; }

    const char *find = std::char_traits<char>::find(m_urlBuffer, m_offset, '?');
    if (!find) { m_urlBuffer[m_offset++] = '?'; }
    else { m_urlBuffer[m_offset++] = '&'; }

    std::copy(param.begin(), param.end(), &m_urlBuffer[m_offset]);
    m_offset += paramLength;

    m_urlBuffer[m_offset++] = '=';

    std::copy(value.begin(), value.end(), &m_urlBuffer[m_offset]);
    m_offset += valueLength;

    return *this;
}

remote::URL &remote::URL::append_slash() noexcept
{
    if (m_offset >= SIZE_URL_BUFFER) { return *this; }
    else if (m_urlBuffer[m_offset] != '/') { m_urlBuffer[m_offset++] = '/'; }

    return *this;
}

const char *remote::URL::get() const noexcept { return m_urlBuffer; }

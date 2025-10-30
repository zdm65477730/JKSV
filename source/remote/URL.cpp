#include "remote/URL.hpp"

#include <algorithm>
#include <span>

//                      ---- Construction ----

remote::URL::URL(std::string_view base) noexcept
{
    const size_t baseLength = base.length();
    if (baseLength >= SIZE_URL_BUFFER) { return; }

    // Create span for this stuff since it's cleaner.
    const std::span<char> bufferSpan{m_urlBuffer, SIZE_URL_BUFFER};

    // Fill and copy.
    std::fill(bufferSpan.begin(), bufferSpan.end(), 0x00);
    std::copy(base.begin(), base.end(), bufferSpan.begin());

    // Update the offset to match.
    m_offset = base.length();
}

//                      ---- Public functions ----

remote::URL &remote::URL::append_path(std::string_view path) noexcept
{
    if (path.empty()) { return *this; }

    const size_t pathLength = path.length();
    if (m_offset + pathLength >= SIZE_URL_BUFFER) { return *this; }

    // Gonna use string view to compare this.
    const std::string_view bufferView{m_urlBuffer};
    if (bufferView.back() != '/' && path.front() != '/') { m_urlBuffer[m_offset++] = '/'; }

    // Gonna use span to copy.
    const size_t remainingSpace = SIZE_URL_BUFFER - m_offset;
    const std::span<char> bufferSpan{&m_urlBuffer[m_offset], remainingSpace};
    std::copy(path.begin(), path.end(), bufferSpan.begin());

    // Update offset.
    m_offset += pathLength;

    return *this;
}

remote::URL &remote::URL::append_parameter(std::string_view param, std::string_view value) noexcept
{
    const size_t paramLength = param.length();
    const size_t valueLength = value.length();
    const size_t endLength   = m_offset + paramLength + valueLength + 2;
    if (endLength >= SIZE_URL_BUFFER) { return *this; }

    // Going to assume if a ? isn't found, we need to append it.
    const char *find = std::char_traits<char>::find(m_urlBuffer, m_offset, '?');
    if (!find) { m_urlBuffer[m_offset++] = '?'; }
    else { m_urlBuffer[m_offset++] = '&'; }

    // Append the parameter first.
    size_t remainingLength = SIZE_URL_BUFFER - m_offset;
    std::span<char> bufferSpan{&m_urlBuffer[m_offset], remainingLength};
    std::copy(param.begin(), param.end(), bufferSpan.begin());
    m_offset += paramLength;

    // Equals.
    m_urlBuffer[m_offset++] = '=';

    // Recalc and align.
    remainingLength = SIZE_URL_BUFFER - m_offset;
    bufferSpan      = std::span<char>{&m_urlBuffer[m_offset], remainingLength};
    std::copy(value.begin(), value.end(), bufferSpan.begin());
    m_offset += valueLength;

    return *this;
}

remote::URL &remote::URL::append_slash() noexcept
{
    if (m_offset >= SIZE_URL_BUFFER) { return *this; }

    // Do a quick check to ensure only one.
    std::string_view urlView{m_urlBuffer};
    if (urlView.back() == '/') { return *this; }

    // Now we append it.
    m_urlBuffer[m_offset++] = '/';

    return *this;
}

const char *remote::URL::get() const noexcept { return m_urlBuffer; }

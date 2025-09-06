#include "remote/URL.hpp"

remote::URL::URL(std::string_view base)
    : m_url(base) {};

remote::URL::URL(const URL &url) { m_url = url.m_url; }

remote::URL::URL(URL &&url) { m_url = std::move(url.m_url); }

remote::URL &remote::URL::operator=(const remote::URL &url)
{
    m_url = url.m_url;
    return *this;
}

remote::URL &remote::URL::operator=(remote::URL &&url) noexcept
{
    m_url = std::move(url.m_url);
    return *this;
}

remote::URL &remote::URL::set_base(std::string_view base)
{
    m_url = base;
    return *this;
}

remote::URL &remote::URL::append_path(std::string_view path)
{
    // Check both just to be sure because this makes WebDav easier to tackle.
    if (m_url.back() != '/' && path.front() != '/') { m_url.append("/"); }
    if (path.empty()) { return *this; }
    m_url.append(path);
    return *this;
}

remote::URL &remote::URL::append_parameter(std::string_view param, std::string_view value)
{
    URL::append_separator();
    m_url.append(param).append("=").append(value);
    return *this;
}

remote::URL &remote::URL::append_slash()
{
    if (m_url.back() != '/') { m_url.append("/"); }
    return *this;
}

const char *remote::URL::get() const noexcept { return m_url.c_str(); }

void remote::URL::append_separator()
{
    if (m_url.find('?') == m_url.npos) { m_url.append("?"); }
    else { m_url.append("&"); }
}

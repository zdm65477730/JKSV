#include "remote/Form.hpp"

remote::Form::Form(const remote::Form &form) { m_form = form.m_form; }

remote::Form::Form(remote::Form &&form) noexcept { m_form = std::move(form.m_form); }

remote::Form &remote::Form::operator=(const remote::Form &form)
{
    m_form = form.m_form;
    return *this;
}

remote::Form &remote::Form::operator=(remote::Form &&form) noexcept
{
    m_form = std::move(form.m_form);
    return *this;
}

remote::Form &remote::Form::append_parameter(std::string_view param, std::string_view value)
{
    if (!m_form.empty() && m_form.back() != '&') { m_form.append("&"); }
    m_form.append(param).append("=").append(value);
    return *this;
}

const char *remote::Form::get() const noexcept { return m_form.c_str(); }

size_t remote::Form::length() const noexcept { return m_form.length(); }

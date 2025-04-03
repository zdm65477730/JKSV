#include "remote/Item.hpp"

remote::Item::Item(std::string_view name, std::string_view id, std::string_view parent, bool directory)
    : m_name(name), m_id(id), m_parent(parent), m_isDirectory(directory) {};

std::string_view remote::Item::get_name(void) const
{
    return m_name;
}

std::string_view remote::Item::get_id(void) const
{
    return m_id;
}

std::string_view remote::Item::get_parent_id(void) const
{
    return m_parent;
}

bool remote::Item::is_directory(void) const
{
    return m_isDirectory;
}

void remote::Item::set_name(std::string_view name)
{
    m_name = name;
}

void remote::Item::set_id(std::string_view id)
{
    m_id = id;
}

void remote::Item::set_parent_id(std::string_view parent)
{
    m_parent = parent;
}

void remote::Item::set_is_directory(bool directory)
{
    m_isDirectory = directory;
}

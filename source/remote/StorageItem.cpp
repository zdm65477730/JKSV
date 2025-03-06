#include "remote/StorageItem.hpp"

remote::StorageItem::StorageItem(std::string_view name,
                                 std::string_view id,
                                 std::string_view parentId,
                                 int size,
                                 bool isDirectory)
    : m_name(name), m_id(id), m_parent(parentId), m_size(size), m_isDirectory(isDirectory) {};

void remote::StorageItem::set_size(int newSize)
{
    m_size = newSize;
}

std::string_view remote::StorageItem::get_name(void) const
{
    return m_name;
}

std::string_view remote::StorageItem::get_id(void) const
{
    return m_id;
}

std::string_view remote::StorageItem::get_parent(void) const
{
    return m_parent;
}

int remote::StorageItem::get_size(void) const
{
    return m_size;
}

bool remote::StorageItem::is_directory(void) const
{
    return m_isDirectory;
}

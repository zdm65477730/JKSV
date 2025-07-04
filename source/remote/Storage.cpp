#include "remote/Storage.hpp"
#include <algorithm>

// Declarations here. Defined at bottom.
remote::Storage::Storage() : m_curl(curl::new_handle()) {};

bool remote::Storage::is_initialized() const
{
    return m_isInitialized;
}

bool remote::Storage::directory_exists(std::string_view name)
{
    return Storage::find_directory_by_name(name) != m_list.end();
}

void remote::Storage::return_to_root()
{
    m_parent = m_root;
}

void remote::Storage::set_root_directory(remote::Item *root)
{
    m_root = root->get_id();
}

void remote::Storage::change_directory(remote::Item *item)
{
    m_parent = item->get_id();
}

remote::Item *remote::Storage::get_directory_by_name(std::string_view name)
{
    auto findDirectory = Storage::find_directory_by_name(name);
    if (findDirectory == m_list.end())
    {
        return nullptr;
    }
    return &(*findDirectory);
}

remote::Storage::DirectoryListing remote::Storage::get_directory_listing()
{
    remote::Storage::DirectoryListing listing;

    Storage::List::iterator current = m_list.begin();
    while ((current = std::find_if(current, m_list.end(), [this](const Item &item) {
                return item.get_parent_id() == this->m_parent;
            })) != m_list.end())
    {
        listing.push_back(&(*current));
    }

    return listing;
}

bool remote::Storage::file_exists(std::string_view name)
{
    return Storage::find_file_by_name(name) != m_list.end();
}

remote::Item *remote::Storage::get_file_by_name(std::string_view name)
{
    auto findFile = Storage::find_file_by_name(name);
    if (findFile == m_list.end())
    {
        return nullptr;
    }
    return &(*findFile);
}

bool remote::Storage::supports_utf8() const
{
    return m_utf8Paths;
}

remote::Storage::List::iterator remote::Storage::find_directory_by_name(std::string_view name)
{
    return std::find_if(m_list.begin(), m_list.end(), [name, this](const Item &item) {
        return item.is_directory() && item.get_parent_id() == this->m_parent && item.get_name() == name;
    });
}

remote::Storage::List::iterator remote::Storage::find_file_by_name(std::string_view name)
{
    return std::find_if(m_list.begin(), m_list.end(), [name, this](const Item &item) {
        return !item.is_directory() && item.get_parent_id() == this->m_parent && item.get_name() == name;
    });
}

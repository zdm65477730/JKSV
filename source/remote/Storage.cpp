#include "remote/Storage.hpp"

#include "logging/logger.hpp"

#include <algorithm>
#include <cstring>

// Declarations here. Defined at bottom.
remote::Storage::Storage(std::string_view prefix, bool supportsUtf8)
    : m_curl(curl::new_handle())
    , m_utf8Paths(supportsUtf8)
    , m_prefix(prefix) {};

bool remote::Storage::is_initialized() const { return m_isInitialized; }

bool remote::Storage::directory_exists(std::string_view name) { return Storage::find_directory_by_name(name) != m_list.end(); }

void remote::Storage::return_to_root() { m_parent = m_root; }

void remote::Storage::set_root_directory(const remote::Item *root) { m_root = root->get_id(); }

void remote::Storage::change_directory(const remote::Item *item) { m_parent = item->get_id(); }

remote::Item *remote::Storage::get_directory_by_name(std::string_view name)
{
    auto findDirectory = Storage::find_directory_by_name(name);
    if (findDirectory == m_list.end()) { return nullptr; }
    return &(*findDirectory);
}

void remote::Storage::get_directory_listing(remote::Storage::DirectoryListing &listOut)
{
    listOut.clear();

    auto current = m_list.begin();
    while ((current = Storage::find_by_parent_id(current, m_parent)) != m_list.end())
    {
        listOut.push_back(&(*current));
        ++current;
    }
}

void remote::Storage::get_directory_listing_with_parent(const remote::Item *item, remote::Storage::DirectoryListing &listOut)
{
    listOut.clear();

    const std::string_view parentId = item->get_id();
    auto current                    = m_list.begin();
    while ((current = Storage::find_by_parent_id(current, parentId)) != m_list.end())
    {
        listOut.push_back(&(*current));
        ++current;
    }
}

bool remote::Storage::file_exists(std::string_view name) { return Storage::find_file_by_name(name) != m_list.end(); }

remote::Item *remote::Storage::get_file_by_name(std::string_view name)
{
    auto findFile = Storage::find_file_by_name(name);
    if (findFile == m_list.end()) { return nullptr; }
    return &(*findFile);
}

bool remote::Storage::supports_utf8() const { return m_utf8Paths; }

std::string_view remote::Storage::get_prefix() const { return m_prefix; }

remote::Storage::List::iterator remote::Storage::find_directory_by_name(std::string_view name)
{
    auto is_match = [&](const Item &item)
    {
        const bool isDir       = item.is_directory();
        const bool parentMatch = isDir && item.get_parent_id() == m_parent;
        const bool nameMatch   = parentMatch && item.get_name() == name;

        return isDir && parentMatch && nameMatch;
    };

    return std::find_if(m_list.begin(), m_list.end(), is_match);
}

remote::Storage::List::iterator remote::Storage::find_directory_by_id(std::string_view id)
{
    auto is_match = [&](const Item &item)
    {
        const bool isDir   = item.is_directory();
        const bool idMatch = item.get_id() == id;
        return isDir && idMatch;
    };

    return std::find_if(m_list.begin(), m_list.end(), is_match);
}

remote::Storage::List::iterator remote::Storage::find_file_by_name(std::string_view name)
{
    auto is_match = [&](const Item &item)
    {
        const bool notDir      = !item.is_directory();
        const bool parentMatch = notDir && item.get_parent_id() == m_parent;
        const bool nameMatch   = parentMatch && item.get_name() == name;

        return notDir && parentMatch && nameMatch;
    };

    return std::find_if(m_list.begin(), m_list.end(), is_match);
}

remote::Storage::List::iterator remote::Storage::find_file_by_id(std::string_view id)
{
    auto is_match = [&](const Item &item)
    {
        const bool isFile  = !item.is_directory();
        const bool isMatch = item.get_id() == id;

        return isFile && isMatch;
    };

    return std::find_if(m_list.begin(), m_list.end(), is_match);
}

remote::Storage::List::iterator remote::Storage::find_item_by_id(std::string_view id)
{
    auto is_match = [&](const Item &item)
    {
        const bool isMatch = item.get_id() == id;

        return isMatch;
    };

    return std::find_if(m_list.begin(), m_list.end(), is_match);
}

remote::Storage::List::iterator remote::Storage::find_by_parent_id(remote::Storage::List::iterator start,
                                                                   std::string_view parentID)
{
    auto is_match = [&](const Item &item) { return item.get_parent_id() == parentID; };

    return std::find_if(start, m_list.end(), is_match);
}

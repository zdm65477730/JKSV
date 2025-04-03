#include "remote/Storage.hpp"
#include <algorithm>

bool remote::Storage::is_initialized(void) const
{
    return m_isInitialized;
}

bool remote::Storage::directory_exists(std::string_view name)
{
    return Storage::find_directory(name) != m_list.end();
}

bool remote::Storage::get_directory_id(std::string_view name, std::string &idOut)
{
    remote::Storage::List::iterator findDir = Storage::find_directory(name);
    if (findDir == m_list.end())
    {
        return false;
    }
    idOut = findDir->get_id();
    return true;
}

bool remote::Storage::file_exists(std::string_view name)
{
    return Storage::find_file(name) != m_list.end();
}

bool remote::Storage::get_file_id(std::string_view name, std::string &idOut)
{
    remote::Storage::List::iterator findFile = Storage::find_file(name);
    if (findFile == m_list.end())
    {
        return false;
    }
    idOut = findFile->get_id();
    return true;
}

remote::Storage::List::iterator remote::Storage::find_directory(std::string_view name)
{
    return std::find_if(m_list.begin(), m_list.end(), [name, this](const Item &item) {
        return item.is_directory() && item.get_parent_id() == this->m_parent && item.get_name() == name;
    });
}

remote::Storage::List::iterator remote::Storage::find_file(std::string_view name)
{
    return std::find_if(m_list.begin(), m_list.end(), [name, this](const Item &item) {
        return !item.is_directory() && item.get_parent_id() == this->m_parent && item.get_name() == name;
    });
}

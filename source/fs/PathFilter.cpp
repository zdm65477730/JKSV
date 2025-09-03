#include "fs/PathFilter.hpp"

#include "JSON.hpp"

fs::PathFilter::PathFilter(const fslib::Path &filePath)
{
    const std::string pathString = filePath.string();
    json::Object filterJSON      = json::new_object(json_object_from_file, pathString.c_str());
    if (!filterJSON) { return; }

    json_object *filter = json::get_object(filterJSON, "filters");
    if (!filter) { return; }

    const size_t arrayLength = json_object_array_length(filter);
    for (size_t i = 0; i < arrayLength; i++)
    {
        json_object *pathObject = json_object_array_get_idx(filter, i);
        if (!pathObject) { break; }

        const char *path = json_object_get_string(pathObject);
        m_paths.emplace_back(path);
    }
}

bool fs::PathFilter::has_paths() const { return !m_paths.empty(); }

bool fs::PathFilter::is_filtered(const fslib::Path &path)
{
    return std::find(m_paths.begin(), m_paths.end(), path) != m_paths.end();
}

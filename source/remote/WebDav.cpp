#include "remote/WebDav.hpp"

#include "JSON.hpp"
#include "curl/curl.hpp"
#include "logger.hpp"
#include "remote/remote.hpp"
#include "strings.hpp"
#include "stringutil.hpp"

#include <tinyxml2.h>

namespace
{
    const char *TAG_XML_HREF = "href";
} // namespace

// Declarations here. Definitions at bottom.
/// @brief Gets a XML element by name. This is namespace agnostic.
/// @param parent Parent XML element.
/// @param name Name of the element to get. No namespace needed.
static tinyxml2::XMLElement *get_element_by_name(tinyxml2::XMLElement *parent, std::string_view name);

/// @brief Returns the starting point of the tag name without the namespace.
/// @param tag Tag to get the name of.
static std::string_view get_tag_begin(std::string_view tag);

/// @brief Slices and unescapes the directory or filename from the href.
/// @param handle Curl handle to use to unescape.
/// @param href HREF to slice.
/// @note This seemed like a better alternative than relying on servers having displayname.
static std::string slice_name_from_href(curl::Handle &handle, std::string_view href);

remote::WebDav::WebDav()
    : Storage("[WD]")
{
    static const char *STRING_CONFIG_READ_ERROR = "Error initializing WebDav: %s";

    json::Object config = json::new_object(json_object_from_file, remote::PATH_WEBDAV_CONFIG.data());
    if (!config)
    {
        logger::log(STRING_CONFIG_READ_ERROR, "Error reading configuration file!");
        return;
    }

    // Let's just get this all out of the way at once.
    json_object *origin   = json::get_object(config, "origin");
    json_object *basepath = json::get_object(config, "basepath");
    json_object *username = json::get_object(config, "username");
    json_object *password = json::get_object(config, "password");

    // This is the bare minimum required to continue.
    if (!origin)
    {
        logger::log(STRING_CONFIG_READ_ERROR, "Config is missing origin!");
        return;
    }
    m_origin = json_object_get_string(origin);

    if (basepath)
    {
        // The root is both in the beginning. I want this to work as closely as the original just not as poorly written
        // or thought out as the original JKSV dav code.
        m_root   = stringutil::get_formatted_string("/%s/", json_object_get_string(basepath));
        m_parent = m_root;
    }

    if (username) { m_username = json_object_get_string(username); }
    if (password) { m_password = json_object_get_string(password); }

    // This is the starting point. This will read the entire basepath listing in one go.
    remote::URL url{m_origin};
    url.append_path(m_root).append_slash();

    // This'll recursively get the full listing of the basepath for the WebDav server.
    std::string xml{};
    if (!WebDav::prop_find(url, xml) || !WebDav::process_listing(xml))
    {
        logger::log(STRING_CONFIG_READ_ERROR, "Error retrieving listing from WebDav server!");
        return;
    }
    m_isInitialized = true;
}

bool remote::WebDav::create_directory(std::string_view name)
{
    static const char *STRING_CREATE_DIR_ERROR = "Error creating WebDav directory: %s";

    std::string escapedName;
    if (!curl::escape_string(m_curl, name, escapedName))
    {
        logger::log(STRING_CREATE_DIR_ERROR, "Error escaping directory name!");
        return false;
    }

    remote::URL url{m_origin};
    url.append_path(m_parent).append_path(escapedName).append_slash();

    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_URL, url.get());
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "MKCOL");

    if (!curl::perform(m_curl)) { return false; }

    if (curl::get_response_code(m_curl) != 201)
    {
        logger::log(STRING_CREATE_DIR_ERROR, name.data());
        return false;
    }

    // This is the ID string so we can make WebDav work within the same framework as Google Drive.
    std::string id = m_parent + "/" + escapedName + "/";
    m_list.emplace_back(name, id, m_parent, 0, true);

    return true;
}

bool remote::WebDav::upload_file(const fslib::Path &source, std::string_view remoteName, sys::ProgressTask *task)
{
    static constexpr const char *STRING_ERROR_UPLOADING = "Error uploading to WebDav: %s";
    const char *statusTemplate                          = strings::get_by_name(strings::names::IO_STATUSES, 5);

    fslib::File sourceFile{source, FsOpenMode_Read};
    if (!sourceFile.is_open())
    {
        logger::log(STRING_ERROR_UPLOADING, fslib::error::get_string());
        return false;
    }

    std::string escapedName;
    if (!curl::escape_string(m_curl, remoteName, escapedName))
    {
        logger::log(STRING_ERROR_UPLOADING, "Failed to escape filename!");
        return false;
    }

    if (task)
    {
        const std::string status = stringutil::get_formatted_string(statusTemplate, remoteName.data());
        task->set_status(status);
        task->reset(static_cast<double>(sourceFile.get_size()));
    }

    remote::URL url{m_origin};
    url.append_path(m_parent).append_path(escapedName);

    curl::UploadStruct uploadData = {.source = &sourceFile, .task = task};

    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_URL, url.get());
    curl::set_option(m_curl, CURLOPT_UPLOAD, 1L);
    curl::set_option(m_curl, CURLOPT_UPLOAD_BUFFERSIZE, Storage::SIZE_UPLOAD_BUFFER);
    curl::set_option(m_curl, CURLOPT_READFUNCTION, curl::read_data_from_file);
    curl::set_option(m_curl, CURLOPT_READDATA, &uploadData);

    if (!curl::perform(m_curl)) { return false; }

    const std::string id = m_parent + "/" + escapedName;
    m_list.emplace_back(remoteName, id, m_parent, sourceFile.get_size(), false);

    return true;
}

bool remote::WebDav::patch_file(remote::Item *item, const fslib::Path &source, sys::ProgressTask *task)
{
    static const char *STRING_ERROR_PATCHING = "Error patching file: %s";

    fslib::File file{source, FsOpenMode_Read};
    if (!file)
    {
        logger::log(STRING_ERROR_PATCHING, fslib::error::get_string());
        return false;
    }

    remote::URL url{m_origin};
    url.append_path(item->get_id());

    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_URL, url.get());
    curl::set_option(m_curl, CURLOPT_UPLOAD, 1L);
    curl::set_option(m_curl, CURLOPT_UPLOAD_BUFFERSIZE, Storage::SIZE_UPLOAD_BUFFER);
    curl::set_option(m_curl, CURLOPT_READFUNCTION, curl::read_data_from_file);
    curl::set_option(m_curl, CURLOPT_READDATA, &file);

    if (!curl::perform(m_curl)) { return false; }

    // Just update the size this time.
    item->set_size(file.get_size());

    return true;
}

bool remote::WebDav::download_file(const remote::Item *item, const fslib::Path &destination, sys::ProgressTask *task)
{
    static const char *STRING_ERROR_DOWNLOADING = "Error downloading file: %s";
    const char *statusTemplate                  = strings::get_by_name(strings::names::IO_STATUSES, 4);

    fslib::File destFile{destination, FsOpenMode_Create | FsOpenMode_Write, item->get_size()};
    if (!destFile)
    {
        logger::log(STRING_ERROR_DOWNLOADING, fslib::error::get_string());
        return false;
    }

    if (task)
    {
        const std::string status = stringutil::get_formatted_string(statusTemplate, item->get_name().data());
        task->set_status(status);
        task->reset(static_cast<double>(item->get_size()));
    }

    remote::URL url{m_origin};
    url.append_path(item->get_id());

    curl::DownloadStruct download{.dest = &destFile, .task = task, .fileSize = item->get_size()};
    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_HTTPGET, 1L);
    curl::set_option(m_curl, CURLOPT_URL, url.get());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::download_file_threaded);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &download);

    std::thread writeThread{curl::download_write_thread_function, std::ref(download)};
    if (!curl::perform(m_curl)) { return false; }
    writeThread.join();

    return true;
}

bool remote::WebDav::delete_item(const remote::Item *item)
{
    static const char *STRING_ERROR_DELETING = "Error deleting item: %s";

    remote::URL url{m_origin};
    url.append_path(item->get_id());
    if (item->is_directory()) { url.append_slash(); }
    logger::log(url.get());

    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl::set_option(m_curl, CURLOPT_URL, url.get());

    if (!curl::perform(m_curl)) { return false; }

    const long code = curl::get_response_code(m_curl);
    if (code != 204) // This can be any of these, I think?
    {
        logger::log(STRING_ERROR_DELETING, "Deletion failed!");
        return false;
    }

    auto findFile = Storage::find_file_by_name(item->get_name());
    if (findFile == m_list.end()) { return false; }

    m_list.erase(findFile);
    return true;
}

void remote::WebDav::append_credentials()
{
    if (!m_username.empty()) { curl::set_option(m_curl, CURLOPT_USERNAME, m_username.c_str()); }
    if (!m_password.empty()) { curl::set_option(m_curl, CURLOPT_PASSWORD, m_password.c_str()); }
}

bool remote::WebDav::prop_find(const remote::URL &url, std::string &xml)
{
    // Some servers block Depth: Infinity.
    curl::HeaderList header = curl::new_header_list();
    curl::append_header(header, "Depth: 1");

    curl::reset_handle(m_curl);
    WebDav::append_credentials();
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "PROPFIND");
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, header.get());
    curl::set_option(m_curl, CURLOPT_URL, url.get());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &xml);

    return curl::perform(m_curl);
}

bool remote::WebDav::process_listing(std::string_view xml)
{
    static const char *STRING_ERROR_PROCESSING_XML = "Error processing XML: %s";

    tinyxml2::XMLDocument listing{};
    if (listing.Parse(xml.data(), xml.length()))
    {
        logger::log(STRING_ERROR_PROCESSING_XML, "Couldn't parse XML!");
        return false;
    }

    tinyxml2::XMLElement *root = listing.RootElement();

    // The first element is what we're using as the parent.
    tinyxml2::XMLElement *parent = root->FirstChildElement();

    tinyxml2::XMLElement *parentLocation = get_element_by_name(parent, TAG_XML_HREF);
    if (!parentLocation)
    {
        logger::log(STRING_ERROR_PROCESSING_XML, "Error finding list parent location!");
        return false;
    }

    // There's no point in continuing if this fails. Just return true.
    tinyxml2::XMLElement *current = parent->NextSiblingElement();
    if (!current) { return true; }

    do {
        // Parsing XML is actually annoying. Even with tinyxml2.
        tinyxml2::XMLElement *href         = get_element_by_name(current, TAG_XML_HREF);
        tinyxml2::XMLElement *propstat     = get_element_by_name(current, "propstat");
        tinyxml2::XMLElement *prop         = get_element_by_name(propstat, "prop");
        tinyxml2::XMLElement *resourceType = get_element_by_name(prop, "resourcetype");
        if (!href || !propstat || !prop || !resourceType)
        {
            logger::log(STRING_ERROR_PROCESSING_XML, "Element is missing data!");
            continue;
        }

        // This is the best way to detect a directory.
        tinyxml2::XMLElement *collection = get_element_by_name(resourceType, "collection");
        if (collection)
        {
            m_list.emplace_back(slice_name_from_href(m_curl, href->GetText()),
                                href->GetText(),
                                parentLocation->GetText(),
                                0,
                                true);

            remote::URL nextUrl{m_origin};
            nextUrl.append_path(href->GetText());

            std::string xml{};
            if (!WebDav::prop_find(nextUrl, xml) || !WebDav::process_listing(xml))
            {
                logger::log(STRING_ERROR_PROCESSING_XML, href->GetText());
            }
        }
        else
        {
            tinyxml2::XMLElement *getContentLength = get_element_by_name(prop, "getcontentlength");
            if (!getContentLength)
            {
                logger::log(STRING_ERROR_PROCESSING_XML, "Missing needed data for file!");
                continue;
            }

            m_list.emplace_back(slice_name_from_href(m_curl, href->GetText()),
                                href->GetText(),
                                parentLocation->GetText(),
                                std::strtoll(getContentLength->GetText(), NULL, 10),
                                false);
        }
    } while ((current = current->NextSiblingElement()));

    return true;
}

static tinyxml2::XMLElement *get_element_by_name(tinyxml2::XMLElement *parent, std::string_view name)
{
    tinyxml2::XMLElement *current = parent->FirstChildElement();
    if (!current) { return nullptr; }

    do {
        if (get_tag_begin(current->Name()) == name) { return current; }
    } while ((current = current->NextSiblingElement()));
    return nullptr;
}

static std::string_view get_tag_begin(std::string_view tag)
{
    size_t colon = tag.find_first_of(':');
    if (colon == tag.npos) { return tag; }
    return tag.substr(colon + 1);
}

static std::string slice_name_from_href(curl::Handle &handle, std::string_view href)
{
    std::string name{};

    // This means we're working with a directory.
    if (href.back() == '/')
    {
        size_t end   = href.find_last_of('/');
        size_t begin = href.find_last_of('/', end - 1);
        if (end == href.npos || begin == href.npos)
        {
            // To do: Maybe handle this better?
            return std::string(href);
        }
        // To do: Inspect this behavior better.
        name = href.substr(begin + 1, (end - begin) - 1);
    }
    else
    {
        // File
        size_t begin = href.find_last_of('/');
        if (begin == href.npos) { name = href; }
        else { name = href.substr(begin + 1); }
    }

    curl::unescape_string(handle, name, name);

    return name;
}

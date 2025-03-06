#include "remote/Storage.hpp"
#include "curl/curl.hpp"
#include "logger.hpp"
#include <algorithm>

namespace
{
    /// @brief This is the buffer size used for uploading files.
    constexpr size_t CURL_UPLOAD_BUFFER_SIZE = 0x10000;
} // namespace

remote::Storage::Storage(void)
{
    m_curl = curl_easy_init();
}

remote::Storage::~Storage()
{
    curl_easy_cleanup(m_curl);
}

bool remote::Storage::is_initialized(void) const
{
    return m_isInitialized;
}

bool remote::Storage::directory_exists(std::string_view directory, std::string_view parentId)
{
    return Storage::find_directory(directory, parentId) != m_remoteList.end();
}

bool remote::Storage::file_exists(std::string_view filename, std::string_view parentId)
{
    return Storage::find_file(filename, parentId) != m_remoteList.end();
}

bool remote::Storage::get_directory_id(std::string_view directoryName, std::string_view parentId, std::string &idOut)
{
    auto findDirectory = Storage::find_directory(directoryName, parentId);
    if (findDirectory == m_remoteList.end())
    {
        return false;
    }
    // Assign the string.
    idOut.assign(findDirectory->get_id());
    return true;
}

bool remote::Storage::get_file_id(std::string_view filename, std::string_view parentId, std::string &idOut)
{
    auto findFile = Storage::find_file(filename, parentId);
    if (findFile == m_remoteList.end())
    {
        return false;
    }
    idOut.assign(findFile->get_id());
    return true;
}

void remote::Storage::curl_prepare_get(void)
{
    // Reset the handle.
    curl_easy_reset(m_curl);

    // Prepare the bare, basic request.
    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_curl, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
    curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "");
}

void remote::Storage::curl_prepare_post(void)
{
    // Reset the handle.
    curl_easy_reset(m_curl);

    // Prepare the post.
    curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, 1L);
    curl_easy_setopt(m_curl, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
}

void remote::Storage::curl_prepare_upload(void)
{
    curl_easy_reset(m_curl);

    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(m_curl, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD_BUFFERSIZE, CURL_UPLOAD_BUFFER_SIZE);
}

void remote::Storage::curl_prepare_patch(void)
{
    curl_easy_reset(m_curl);

    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(m_curl, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD_BUFFERSIZE, CURL_UPLOAD_BUFFER_SIZE);
}

bool remote::Storage::curl_perform(void)
{
    int curlError = curl_easy_perform(m_curl);
    if (curlError != CURLE_OK)
    {
        logger::log("remote::Storage: Error performing curl: %i.", curlError);
        return false;
    }
    return true;
}

std::vector<remote::StorageItem>::iterator remote::Storage::find_directory(std::string_view directoryName,
                                                                           std::string_view parentId)
{
    return std::find_if(m_remoteList.begin(), m_remoteList.end(), [directoryName, parentId](auto &item) {
        return item.is_directory() && item.get_name() == directoryName && item.get_parent() == parentId;
    });
}

std::vector<remote::StorageItem>::iterator remote::Storage::find_file(std::string_view filename,
                                                                      std::string_view parentId)
{
    return std::find_if(m_remoteList.begin(), m_remoteList.end(), [filename, parentId](auto &item) {
        return !item.is_directory() && item.get_name() == filename && item.get_parent() == parentId;
    });
}

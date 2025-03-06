#include "curl.hpp"
#include "logger.hpp"
#include "stringutil.hpp"

// Declarations here. Definitions later.
// Wraps a curl handle in a self freeing unique_ptr
using CurlHandle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;

// Creates a new curl handle.
static CurlHandle new_curl_handle(void);
// This function prepares the base of a get request so I don't have to type so much.
static void prepare_get_request(CURL *curl);

size_t curl::read_from_file(char *buffer, size_t size, size_t count, fslib::File *target)
{
    // This just needs to read and return what it already returns.
    return target->read(buffer, size * count);
}

size_t curl::write_headers_array(const char *buffer, size_t size, size_t count, curl::HeaderArray *headerArray)
{
    // Just push the header back.
    headerArray->emplace_back(buffer);
    // Return this so curl thinks it worked cause it probably totally did.
    return size * count;
}

size_t curl::write_response_string(const char *buffer, size_t size, size_t count, std::string *string)
{
    // Append the buffer to the string.
    string->append(buffer);
    // Return success
    return size * count;
}

size_t curl::write_response_buffer(const char *buffer, size_t size, size_t count, curl::DownloadBuffer *bufferOut)
{
    // Insert the data to the end of the vector.
    bufferOut->insert(bufferOut->end(), reinterpret_cast<unsigned char *>(buffer), size * count);
    // Return success.
    return size * count;
}

bool curl::download_to_string(std::string_view url, std::string &stringOut)
{
    // Curl handle.
    CurlHandle curlHandle = new_curl_handle();

    // Setup get request.
    prepare_get_request(curlHandle.get());
    curl_easy_setopt(curlHandle.get(), CURLOPT_URL, url.data());
    curl_easy_setopt(curlHandle.get(), CURLOPT_WRITEFUNCTION, write_response_string);
    curl_easy_setopt(curlHandle.get(), CURLOPT_WRITEDATA, &stringOut);

    // Clear the string first just in case.
    stringOut.clear();

    if (curl_easy_perform(curlHandle.get()) != CURLE_OK)
    {
        logger::log("Error getting %s.", url.data());
        return false;
    }

    return true;
}

bool curl::download_to_buffer(std::string_view url, curl::DownloadBuffer &bufferOut)
{
    CurlHandle curlHandle = new_curl_handle();

    prepare_get_request(curlHandle.get());
    curl_easy_setopt(curlHandle.get(), CURLOPT_URL, url.data());
    curl_easy_setopt(curlHandle.get(), CURLOPT_WRITEFUNCTION, write_response_buffer);
    curl_easy_setopt(curlHandle.get(), CURLOPT_WRITEDATA, &bufferOut);

    // Clear the vector first.
    bufferOut.clear();

    if (curl_easy_perform(curlHandle.get()) != CURLE_OK)
    {
        logger::log("Error downloading %s.", url.data());
        return false;
    }

    return true;
}

bool curl::extract_value_from_headers(const curl::HeaderArray &headerArray,
                                      std::string_view header,
                                      std::string &valueOut)
{
    for (const auto &currentHeader : headerArray)
    {
        // Check if the current header matches what we need.
        if (currentHeader.find(header.data()) != currentHeader.end())
        {
            // It does. Find the ':'.
            size_t headerEnd = currentHeader.find_first_of(':') + 2;

            // This should get what's after the ':'.
            valueOut = currentHeader.substr(headerEnd, -1);

            // Strip and newline chars.
            stringutil::strip_character('\n', valueOut);
            stringutil::strip_character('\r', valueOut);

            return true;
        }
    }
    return false;
}

static CurlHandle new_curl_handle(void)
{
    return CurlHandle(curl_easy_init, curl_easy_cleanup);
}

static void prepare_get_request(CURL *curl)
{
    // This is just the basics. The other functions fill in the rest.
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
}

#include "curl.hpp"
#include "stringutil.hpp"

namespace
{
    /// @brief Size of the buffer used for uploading files.
    constexpr size_t SIZE_UPLOAD_BUFFER = 0x10000;
} // namespace

size_t curl::read_data_from_file(char *buffer, size_t size, size_t count, fslib::File *target)
{
    // This should be good enough.
    return target->read(buffer, size * count);
}

size_t curl::write_header_array(const char *buffer, size_t size, size_t count, curl::HeaderArray *array)
{
    array->emplace_back(buffer, buffer + (size * count));
    return size * count;
}

size_t curl::write_response_string(const char *buffer, size_t, size_t count, std::string *string)
{
    string->append(buffer, buffer + (size * count));
    return size * count;
}

bool curl::get_header_value(const curl::HeaderArray &array, std::string_view header, std::string &valueOut)
{
    for (const std::string &currentHeader : array)
    {
        size_t colonPos = currentHeader.find_first_of(':');
        if (colonPos == currentHeader.npos)
        {
            continue;
        }

        // Get the substr.
        std::string headerName = currentHeader.substr(0, colonPos);
        if (headerName != header)
        {
            continue;
        }

        // Find the first thing after that isn't a space.
        size_t valueBegin = currentHeader.find_first_not_of(' ', colonPos + 1);
        if (valueBegin == currentHeader.npos)
        {
            // There was no value found?
            continue;
        }

        // Set value out and return true. Npos should auto to the end. Strip new lines if there are any.
        valueOut = currentHeader.substr(valueBegin, currentHeader.npos);
        stringutil::strip_character('\n', valueOut);
        stringutil::strip_character('\r', valueOut);

        return true;
    }
    return false;
}

void curl::prepare_get(curl::Handle &curl)
{
    // Reset handle. This is faster than making duplicates over and over.
    curl_easy_reset(curl.get());

    // Setup basic request.
    curl::set_option(curl, CURLOPT_HTTPGET, 1L);
    curl::set_option(curl, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
    curl::set_option(curl, CURLOPT_ACCEPT_ENCODING, "") // I think this is how you set the defaults for this?
}

void curl::prepare_post(curl::Handle &curl)
{
    curl_easy_reset(curl.get());

    curl::set_option(curl, CURLOPT_POST, 1L);
    curl::set_option(curl, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
    curl::set_option(curl, CURLOPT_ACCEPT_ENCODING, "");
}

void curl::prepare_upload(curl::Handle &curl)
{
    curl_easy_reset(curl.get());

    curl::set_option(curl, CURLOPT_UPLOAD, 1L);
    curl::set_option(curl, CURLOPT_USERAGENT, curl::USER_AGENT_STRING.data());
    curl::set_option(curl, CURLOPT_UPLOAD_BUFFERSIZE, SIZE_UPLOAD_BUFFER);
    curl::set_option(curl, CURLOPT_ACCEPT_ENCODING, "") // Not really sure this will have any affect here...
}
`

#include "curl/curl.hpp"

#include "error.hpp"
#include "logger.hpp"
#include "stringutil.hpp"

namespace
{
    /// @brief Size of the buffer used for uploading files.
    constexpr size_t SIZE_UPLOAD_BUFFER = 0x10000;
} // namespace

bool curl::initialize() { return curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK; }

void curl::exit() { curl_global_cleanup(); }

bool curl::perform(curl::Handle &handle)
{
    CURLcode error = curl_easy_perform(handle.get());
    if (error != CURLE_OK)
    {
        logger::log("Error performing curl: %i.", error);
        return false;
    }
    return true;
}

void curl::append_header(curl::HeaderList &list, std::string_view header)
{
    // This is the only real way to accomplish this since slist is a linked list.
    curl_slist *head = list.release();
    head             = curl_slist_append(head, header.data());
    list.reset(head);
}

size_t curl::read_data_from_file(char *buffer, size_t size, size_t count, curl::UploadStruct *upload)
{
    if (error::is_null(upload)) { return -1; }
    fslib::File *source     = upload->source;
    sys::ProgressTask *task = upload->task;

    ssize_t readSize = source->read(buffer, size * count);
    if (task) { task->update_current(static_cast<double>(source->tell())); }

    return readSize;
}

size_t curl::write_header_array(const char *buffer, size_t size, size_t count, curl::HeaderArray *array)
{
    array->emplace_back(buffer, buffer + (size * count));
    return size * count;
}

size_t curl::write_response_string(const char *buffer, size_t size, size_t count, std::string *string)
{
    string->append(buffer, size * count);
    return size * count;
}

size_t curl::write_data_to_file(const char *buffer, size_t size, size_t count, fslib::File *target)
{
    return target->write(buffer, size * count);
}

bool curl::get_header_value(const curl::HeaderArray &array, std::string_view header, std::string &valueOut)
{
    for (const std::string &currentHeader : array)
    {
        size_t colonPos = currentHeader.find_first_of(':');
        if (colonPos == currentHeader.npos) { continue; }

        // Get the substr.
        std::string headerName = currentHeader.substr(0, colonPos);
        if (headerName != header) { continue; }

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

long curl::get_response_code(curl::Handle &handle)
{
    long code = 0;
    curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &code);
    return code;
}

bool curl::escape_string(curl::Handle &handle, std::string_view in, std::string &out)
{
    char *escaped = curl_easy_escape(handle.get(), in.data(), in.length());
    if (!escaped) { return false; }

    out.assign(escaped);

    // I'm assuming this just calls free itself?
    curl_free(escaped);
    return true;
}

bool curl::unescape_string(curl::Handle &handle, std::string_view in, std::string &out)
{
    int lengthOut{};
    char *unescaped = curl_easy_unescape(handle.get(), in.data(), in.length(), &lengthOut);
    if (!unescaped) { return false; }

    out.assign(unescaped);

    curl_free(unescaped);
    return true;
}

void curl::prepare_get(curl::Handle &curl)
{
    // Reset handle. This is faster than making duplicates over and over.
    curl::reset_handle(curl);

    // Setup basic request.
    curl::set_option(curl, CURLOPT_HTTPGET, 1L);
    curl::set_option(curl, CURLOPT_ACCEPT_ENCODING, ""); // I think this is how you set the defaults for this?
}

void curl::prepare_post(curl::Handle &curl)
{
    curl::reset_handle(curl);

    curl::set_option(curl, CURLOPT_POST, 1L);
    curl::set_option(curl, CURLOPT_ACCEPT_ENCODING, "");
}

void curl::prepare_upload(curl::Handle &curl)
{
    curl::reset_handle(curl);

    curl::set_option(curl, CURLOPT_UPLOAD, 1L);
    curl::set_option(curl, CURLOPT_ACCEPT_ENCODING, ""); // Not really sure this will have any affect here...
}

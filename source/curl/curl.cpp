#include "curl/curl.hpp"

#include "error.hpp"
#include "logging/logger.hpp"
#include "stringutil.hpp"

#include <cstring>

namespace
{
    /// @brief Size of the buffer used for uploading files.
    constexpr size_t SIZE_UPLOAD_BUFFER      = 0x10000;
    constexpr size_t SIZE_DOWNLOAD_THRESHOLD = 0x400000;
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

size_t curl::download_file_threaded(const char *buffer, size_t size, size_t count, curl::DownloadStruct *download)
{
    std::mutex &lock                   = download->lock;
    std::condition_variable &condition = download->condition;
    auto &sharedBuffer                 = download->sharedBuffer;
    size_t &sharedOffset               = download->sharedOffset;
    bool &bufferReady                  = download->bufferReady;
    sys::ProgressTask *task            = download->task;
    size_t &offset                     = download->offset;
    int64_t &fileSize                  = download->fileSize;

    const size_t downloadSize = size * count;
    const std::span<const sys::byte> bufferSpan{reinterpret_cast<const sys::byte *>(buffer), downloadSize};

    {
        std::unique_lock<std::mutex> bufferLock(lock);
        condition.wait(bufferLock, [&]() { return bufferReady == false; });

        std::copy(bufferSpan.begin(), bufferSpan.end(), &sharedBuffer[sharedOffset]);
        sharedOffset += downloadSize;

        const int64_t nextOffset = offset + downloadSize;
        if (sharedOffset >= SIZE_DOWNLOAD_THRESHOLD || nextOffset >= fileSize)
        {
            bufferReady = true;
            condition.notify_one();
        }

        offset += downloadSize;
    }

    if (task) { task->increase_current(static_cast<double>(downloadSize)); }

    return downloadSize;
}

void curl::download_write_thread_function(sys::threadpool::JobData jobData)
{
    auto castData = std::static_pointer_cast<curl::DownloadStruct>(jobData);

    std::mutex &lock                   = castData->lock;
    std::condition_variable &condition = castData->condition;
    auto &sharedBuffer                 = castData->sharedBuffer;
    size_t &sharedOffset               = castData->sharedOffset;
    bool &bufferReady                  = castData->bufferReady;
    fslib::File &dest                  = *castData->dest;
    size_t fileSize                    = castData->fileSize;

    auto localBuffer = std::make_unique<sys::byte[]>(SIZE_DOWNLOAD_THRESHOLD + 0x100000); // Gonna give this some room.

    for (size_t i = 0; i < fileSize;)
    {
        size_t bufferSize{};
        {
            std::unique_lock<std::mutex> bufferLock(lock);
            condition.wait(bufferLock, [&]() { return bufferReady == true; });

            // Copy and reset the offset.
            bufferSize = sharedOffset;
            std::copy(sharedBuffer.begin(), sharedBuffer.begin() + sharedOffset, localBuffer.get());
            sharedOffset = 0;

            bufferReady = false;
            condition.notify_one();
        }
        dest.write(localBuffer.get(), bufferSize);
        i += bufferSize;
    }
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
    // curl::set_option(curl, CURLOPT_ACCEPT_ENCODING, "");
}

void curl::prepare_post(curl::Handle &curl)
{
    curl::reset_handle(curl);

    curl::set_option(curl, CURLOPT_POST, 1L);
    // curl::set_option(curl, CURLOPT_ACCEPT_ENCODING, "");
}

void curl::prepare_upload(curl::Handle &curl)
{
    curl::reset_handle(curl);

    curl::set_option(curl, CURLOPT_UPLOAD, 1L);
    // curl::set_option(curl, CURLOPT_ACCEPT_ENCODING, "");
}

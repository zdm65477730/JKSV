#pragma once
#include "fslib.hpp"
#include <condition_variable>
#include <curl/curl.h>
#include <memory>
#include <mutex>
#include <vector>

namespace curl
{

    /// @brief Universal user agent string used by JKSV for everything.
    static constexpr std::string_view USER_AGENT_STRING = "JKSV";

    /// @brief Used to store headers when needed.
    using HeaderArray = std::vector<std::string>;

    /// @brief Download buffer declaration.
    using DownloadBuffer = std::vector<unsigned char>;

    /// @brief This is the struct used for threaded downloads from drive.
    typedef struct
    {
            /// @brief Conditional used for mutex.
            std::condition_variable m_condition;
            /// @brief Buffer mutex.
            std::mutex m_bufferLock;
            /// @brief Buffer both threads share.
            std::unique_ptr<unsigned char[]> m_sharedBuffer;
            /// @brief Path the to file to write to.
            fslib::Path m_filePath;
            /// @brief Size of the file being downloaded.
            uint64_t m_fileSize;
            /// @brief Current offset of the downloaded file.
            uint64_t m_offset;
    } DownloadStruct;

    /// @brief This makes some stuff slightly easier to read. Also, I don't need to type as much.
    using SharedDownloadStruct = std::shared_ptr<DownloadStruct>;

    /// @brief Reads data from the target file to upload it.
    /// @param buffer Incoming buffer from curl.
    /// @param size Element size.
    /// @param count Element count.
    /// @param target Target file to read from.
    /// @return Number of bytes read.
    size_t read_from_file(char *buffer, size_t size, size_t count, fslib::File *target);

    /// @brief Curl callback function that stores headers to a vector (curl::HeaderArray)
    /// @param buffer Incoming buffer from curl.
    /// @param size Size of the elements.
    /// @param count Element count.
    /// @param headerArray Vector to write to.
    /// @return Number of bytes in the buffer.
    size_t write_headers_array(const char *buffer, size_t size, size_t count, curl::HeaderArray *headerArray);

    /// @brief Curl callback function to write the response to a C++ string.
    /// @param buffer Buffer from curl
    /// @param size Element size.
    /// @param count Element count.
    /// @param string String to write to.
    /// @return Size written to string.
    size_t write_response_string(const char *buffer, size_t size, size_t count, std::string *string);

    /// @brief Writes the response to a download buffer (std::vector<unsigned char>).
    /// @param buffer Buffer from curl.
    /// @param size Element size.
    /// @param count Element count.
    /// @param bufferOut Buffer to write data to.
    /// @return Size written to buffer.
    size_t write_response_buffer(const char *buffer, size_t size, size_t count, curl::DownloadBuffer *bufferOut);

    /// @brief Extracts the value of a header from the vector passed.
    /// @param headerArray Vector of headers to search for the value in.
    /// @param header Header to find and get the value of.
    /// @param valueOut String to write the value to.
    /// @return True if the value was found. False if it wasn't.
    bool extract_value_from_headers(const curl::HeaderArray &headerArray,
                                    std::string_view header,
                                    std::string &valueOut);

    /// @brief Quick shortcut function for a get request that writes the response to a C++ string.
    /// @param url Url to GET.
    /// @param stringOut String to write response to.
    /// @return True on success. False on failure.
    bool download_to_string(std::string_view url, std::string &stringOut);

    /// @brief Quick shortcut function to get and download something to a buffer.
    /// @param url URL to get and download.
    /// @param buffer Buffer to write response to.
    /// @return True on success. False on failure.
    bool download_to_buffer(std::string_view url, curl::DownloadBuffer &bufferOut);

} // namespace curl

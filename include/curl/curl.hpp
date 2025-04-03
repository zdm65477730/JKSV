#pragma once
#include "fslib.hpp"
#include "logger.hpp"
#include <curl/curl.h>
#include <memory>
#include <string>
#include <vector>

/// @brief This namespace contains various wrapped libcurl functions and data.
namespace curl
{
    /// @brief JKSV's user agent string.
    static constexpr std::string_view USER_AGENT_STRING = "JKSV";

    /// @brief Self cleaning curl handle.
    using Handle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;

    /// @brief Self cleaning curl header/slist.
    using HeaderList = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;

    /// @brief Definition for a vector containing headers received from libcurl.
    using HeaderArray = std::vector<std::string>;

    /// @brief Initializes lib curl.
    /// @return True on success. False on failure.
    bool initialize(void);

    /// @brief Exits libcurl
    void exit(void);

    /// @brief Inline function that returns a self cleaning curl handle.
    /// @return Curl handle.
    static inline curl::Handle new_handle(void)
    {
        return curl::Handle(curl_easy_init(), curl_easy_cleanup);
    }

    /// @brief Inline wrapper function for curl_easy_reset.
    /// @param curl curl::Handle to reset.
    static inline void reset_handle(curl::Handle &curl)
    {
        curl_easy_reset(curl.get());
    }

    /// @brief Logged inline wrapper function for curl_easy_perform.
    /// @param curl Handle to perform.
    /// @return True on success. False on failure.
    static inline bool perform(curl::Handle &curl)
    {
        // CURLCode is just an int anyway.
        CURLcode error = curl_easy_perform(curl.get());
        if (error != CURLE_OK)
        {
            logger::log("Error performing curl: %i.", error);
            return false;
        }
        return true;
    }

    /// @brief Inline templated function to wrap curl_easy_setopt and make using curl::Handle slightly easier.
    /// @tparam Option Templated type of the option. This is a headache so let the compiler figure it out.
    /// @tparam Value Templated type of the value to set the option too. See above.
    /// @param handle curl::Handle the option is being set for.
    /// @param option CURLOPT to set.
    /// @param value Value to set the CURLOPT to.
    /// @return CURLcode returned from curl_easy_setopt.
    template <typename Option, typename Value>
    static inline CURLcode set_option(curl::Handle &handle, Option option, Value value)
    {
        return curl_easy_setopt(handle.get(), option, value);
    }

    /// @brief Inline wrapper function to make adding to HeaderList simpler.
    /// @param headerList Header list to append to.
    /// @param header Header to append.
    static inline void append_header(curl::HeaderList &headerList, std::string_view header)
    {
        // Release the current list and save the pointer to the head of it.
        curl_slist *list = headerList.release();

        // Append to it.
        curl_slist_append(list, header.data());

        // Reassign the unique_ptr to the new head.
        headerList.reset(list);
    }

    /// @brief Curl callback function for reading data from a file.
    /// @param buffer Incoming buffer from curl to read to.
    /// @param size Element size.
    /// @param count Element count.
    /// @param target Target file to read from.
    /// @return Number of bytes read so curl thinks everything went OK.
    size_t read_data_from_file(char *buffer, size_t size, size_t count, fslib::File *target);

    /// @brief Curl callback function that writes incoming headers to a vector/array.
    /// @param buffer Incoming buffer from curl.
    /// @param size Element size.
    /// @param count Element count.
    /// @param array Array to write the header to.
    /// @return size * count so curl thinks everything is fine, because it usually is.
    size_t write_header_array(const char *buffer, size_t size, size_t count, curl::HeaderArray *array);

    /// @brief Curl callback function that writes the response data to a C++ string.
    /// @param buffer Incoming buffer from curl.
    /// @param size Element size.
    /// @param count Element count.
    /// @param string String to write the response to.
    /// @return size * count so curl thinks everything is fine.
    size_t write_response_string(const char *buffer, size_t size, size_t count, std::string *string);

    /// @brief Gets the value of a header from an array of headers.
    /// @param array Array of headers to search.
    /// @param header Header to search for.
    /// @param valueOut String to write the value to.
    /// @return True if the header was found and the value was successfully extracted.
    bool get_header_value(const curl::HeaderArray &array, std::string_view header, std::string &valueOut);

    /// @brief Prepares the curl handle passed for a get request.
    /// @param curl Handle to prepare for a get request.
    void prepare_get(curl::Handle &curl);

    /// @brief Prepares the curl handle for a post request.
    /// @param curl Handle prepare for a post request.
    void prepare_post(curl::Handle &curl);

    /// @brief Prepares the curl handle for an upload.
    /// @param curl Handle to reset and prepare for an upload.
    void prepare_upload(curl::Handle &curl);
} // namespace curl

#include "stringutil.hpp"

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <string>
#include <switch.h>

namespace
{
    // Size limit for formatted strings.
    constexpr size_t VA_BUFFER_SIZE = 0x1000;
    // These characters get replaced by spaces when path is sanitized.
    constexpr std::array<uint32_t, 13> FORBIDDEN_PATH_CHARACTERS =
        {L',', L'/', L'\\', L'<', L'>', L':', L'"', L'|', L'?', L'*', L'™', L'©', L'®'};
} // namespace

std::string stringutil::get_formatted_string(const char *format, ...)
{
    std::array<char, VA_BUFFER_SIZE> vaBuffer = {0};

    std::va_list vaList;
    va_start(vaList, format);
    vsnprintf(vaBuffer.data(), VA_BUFFER_SIZE, format, vaList);
    va_end(vaList);

    return std::string(vaBuffer.data());
}

void stringutil::replace_in_string(std::string &target, std::string_view find, std::string_view replace)
{
    const size_t findLength    = find.length();
    const size_t replaceLength = replace.length();

    for (size_t i = target.find(find); i != target.npos; i = target.find(find, i + replaceLength))
    {
        target.replace(i, findLength, replace);
    }
}

void stringutil::strip_character(char c, std::string &target)
{
    for (size_t i = target.find_first_of(c); i != target.npos; i = target.find_first_of(c, i))
    {
        target.erase(target.begin() + i);
    }
}

bool stringutil::sanitize_string_for_path(const char *stringIn, char *stringOut, size_t stringOutSize)
{
    uint32_t codepoint{};
    const int length = std::char_traits<char>::length(stringIn);

    for (int i = 0, outOffset = 0; i < length;)
    {
        const uint8_t *point = reinterpret_cast<const uint8_t *>(&stringIn[i]);
        const ssize_t count  = decode_utf8(&codepoint, point);
        if (count <= 0 || i + count >= static_cast<int>(stringOutSize)) { return false; }

        if (codepoint == L'é')
        {
            stringOut[outOffset++] = 'e';
            i += count;
            continue;
        }

        const bool asciiCheck = codepoint < 0x1E || codepoint >= 0x7E;
        if (asciiCheck) { return false; }

        const bool isForbidden = std::find(FORBIDDEN_PATH_CHARACTERS.begin(), FORBIDDEN_PATH_CHARACTERS.end(), codepoint) !=
                                 FORBIDDEN_PATH_CHARACTERS.end();
        if (isForbidden)
        {
            i += count;
            continue;
        }
        else
        {
            std::memcpy(&stringOut[outOffset], &stringIn[i], static_cast<size_t>(count));
            outOffset += count;
        }

        i += count;
    }

    const int outLength = std::char_traits<char>::length(stringOut) - 1;
    for (int i = outLength; i > 0 && (stringOut[i] == ' ' || stringOut[i] == '.'); i--) { stringOut[i] = '\0'; }

    return true;
}

std::string stringutil::get_date_string(stringutil::DateFormat format)
{
    char stringBuffer[0x80] = {0};

    std::time_t timer{};
    std::time(&timer);
    const std::tm *localTime = std::localtime(&timer);

    switch (format)
    {
        case stringutil::DateFormat::YearMonthDay:
        {
            std::strftime(stringBuffer, 0x80, "%Y-%m-%d_%H-%M-%S", localTime);
        }
        break;

        case stringutil::DateFormat::YearDayMonth:
        {
            std::strftime(stringBuffer, 0x80, "%Y-%d-%m_%H-%M-%S", localTime);
        }
        break;
    }

    return std::string(stringBuffer);
}

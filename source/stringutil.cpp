#include "stringutil.hpp"
#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <switch.h>

namespace
{
    // Size limit for formatted strings.
    constexpr size_t VA_BUFFER_SIZE = 0x1000;
    // These characters get replaced by spaces when path is sanitized.
    constexpr std::array<uint32_t, 13> FORBIDDEN_PATH_CHARACTERS =
        {L',', L'/', L'\\', L'<', L'>', L':', L'"', L'|', L'?', L'*', L'™', L'©', L'®'};
} // namespace

std::string stringutil::getFormattedString(const char *format, ...)
{
    char vaBuffer[VA_BUFFER_SIZE] = {0};

    std::va_list vaList;
    va_start(vaList, format);
    vsnprintf(vaBuffer, VA_BUFFER_SIZE, format, vaList);
    va_end(vaList);

    return std::string(vaBuffer);
}

void stringutil::replaceInString(std::string &target, std::string_view find, std::string_view replace)
{
    size_t stringPosition = 0;
    while ((stringPosition = target.find(find, stringPosition)) != target.npos)
    {
        target.replace(stringPosition, find.length(), replace);
    }
}

bool stringutil::sanitizeStringForPath(const char *stringIn, char *stringOut, size_t stringOutSize)
{
    uint32_t codepoint = 0;
    size_t stringLength = std::strlen(stringIn);
    for (size_t i = 0, stringOutOffset = 0; i < stringLength;)
    {
        ssize_t unitCount = decode_utf8(&codepoint, reinterpret_cast<const uint8_t *>(&stringIn[i]));
        if (unitCount <= 0 || i + unitCount >= stringOutSize)
        {
            break;
        }

        if (codepoint < 0x20 || codepoint > 0x7E)
        {
            // Don't even bother. It's not possible.
            return false;
        }

        // replace forbidden with spaces.
        if (std::find(FORBIDDEN_PATH_CHARACTERS.begin(), FORBIDDEN_PATH_CHARACTERS.end(), codepoint) != FORBIDDEN_PATH_CHARACTERS.end())
        {
            stringOut[stringOutOffset++] = 0x20;
        }
        else if (codepoint == L'é')
        {
            stringOut[stringOutOffset++] = 'e';
        }
        else
        {
            // Just memcpy it over. This is a safety thing to be honest. Since it's only Ascii allowed, unitcount should only be 1.
            std::memcpy(&stringOut[stringOutOffset], &stringIn[i], static_cast<size_t>(unitCount));
            stringOutOffset += unitCount;
        }
        i += unitCount;
    }

    // Loop backwards and trim off spaces and periods.
    size_t stringOutLength = std::strlen(stringOut);
    while (stringOut[stringOutLength - 1] == ' ' || stringOut[stringOutLength - 1] == '.')
    {
        stringOut[--stringOutLength] = 0x00;
    }
    return true;
}

std::string stringutil::getDateString(stringutil::DateFormat format)
{
    char stringBuffer[0x80] = {0};

    std::time_t timer;
    std::time(&timer);
    std::tm *localTime = std::localtime(&timer);

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

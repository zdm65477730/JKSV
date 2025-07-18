#include "strings.hpp"
#include "JSON.hpp"
#include "fslib.hpp"
#include "stringutil.hpp"
#include <map>
#include <string>
#include <unordered_map>

namespace
{
    // This is the actual map where the strings are.
    std::map<std::pair<std::string, int>, std::string> s_stringMap;
    // This map is for matching files to the language value
    std::unordered_map<SetLanguage, std::string_view> s_fileMap = {{SetLanguage_JA, "JA.json"},
                                                                   {SetLanguage_ENUS, "ENUS.json"},
                                                                   {SetLanguage_FR, "FR.json"},
                                                                   {SetLanguage_DE, "DE.json"},
                                                                   {SetLanguage_IT, "IT.json"},
                                                                   {SetLanguage_ES, "ES.json"},
                                                                   {SetLanguage_ZHCN, "ZHCN.json"},
                                                                   {SetLanguage_KO, "KO.json"},
                                                                   {SetLanguage_NL, "NL.json"},
                                                                   {SetLanguage_PT, "PT.json"},
                                                                   {SetLanguage_RU, "RU.json"},
                                                                   {SetLanguage_ZHTW, "ZHTW.json"},
                                                                   {SetLanguage_ENGB, "ENGB.json"},
                                                                   {SetLanguage_FRCA, "FRCA.json"},
                                                                   {SetLanguage_ES419, "ES419.json"},
                                                                   {SetLanguage_ZHHANS, "ZHCN.json"},
                                                                   {SetLanguage_ZHHANT, "ZHTW.json"},
                                                                   {SetLanguage_PTBR, "PTBR.json"}};
} // namespace

// This returns the language file to use depending on the system's language.
static fslib::Path get_file_path()
{
    fslib::Path returnPath = "romfs:/Text";

    uint64_t languageCode = 0;
    Result setError = setGetLanguageCode(&languageCode);
    if (R_FAILED(setError))
    {
        return returnPath / s_fileMap.at(SetLanguage_ENUS);
    }

    SetLanguage language;
    setError = setMakeLanguage(languageCode, &language);
    if (R_FAILED(setError))
    {
        return returnPath / s_fileMap.at(SetLanguage_ENUS);
    }
    return returnPath / s_fileMap.at(language);
}

static void replace_buttons_in_string(std::string &target)
{
    stringutil::replace_in_string(target, "[A]", "\ue0e0");
    stringutil::replace_in_string(target, "[B]", "\ue0e1");
    stringutil::replace_in_string(target, "[X]", "\ue0e2");
    stringutil::replace_in_string(target, "[Y]", "\ue0e3");
    stringutil::replace_in_string(target, "[L]", "\ue0e4");
    stringutil::replace_in_string(target, "[R]", "\ue0e5");
    stringutil::replace_in_string(target, "[ZL]", "\ue0e6");
    stringutil::replace_in_string(target, "[ZR]", "\ue0e7");
    stringutil::replace_in_string(target, "[SL]", "\ue0e8");
    stringutil::replace_in_string(target, "[SR]", "\ue0e9");
    stringutil::replace_in_string(target, "[DPAD]", "\ue0ea");
    stringutil::replace_in_string(target, "[DUP]", "\ue0eb");
    stringutil::replace_in_string(target, "[DDOWN]", "\ue0ec");
    stringutil::replace_in_string(target, "[DLEFT]", "\ue0ed");
    stringutil::replace_in_string(target, "[DRIGHT]", "\ue0ee");
    stringutil::replace_in_string(target, "[+]", "\ue0ef");
    stringutil::replace_in_string(target, "[-]", "\ue0f0");
}

bool strings::initialize()
{
    fslib::Path filePath = get_file_path();

    json::Object stringJSON = json::new_object(json_object_from_file, filePath.full_path());
    if (!stringJSON)
    {
        return false;
    }

    json_object_iterator stringIterator = json_object_iter_begin(stringJSON.get());
    json_object_iterator stringEnd = json_object_iter_end(stringJSON.get());
    while (!json_object_iter_equal(&stringIterator, &stringEnd))
    {
        // Get name of string(s) and pointer to array
        const char *stringName = json_object_iter_peek_name(&stringIterator);
        json_object *stringArray = json_object_iter_peek_value(&stringIterator);

        // Loop through array and add them to map so I can be lazier and not have to edit code or do shit to add more strings.
        size_t arrayLength = json_object_array_length(stringArray);
        for (size_t i = 0; i < arrayLength; i++)
        {
            json_object *string = json_object_array_get_idx(stringArray, i);
            s_stringMap[std::make_pair(stringName, static_cast<int>(i))] = json_object_get_string(string);
        }
        json_object_iter_next(&stringIterator);
    }

    // Loop through entire map and replace the buttons.
    for (auto &[key, string] : s_stringMap)
    {
        replace_buttons_in_string(string);
    }

    return true;
}

const char *strings::get_by_name(std::string_view name, int index)
{
    if (s_stringMap.find(std::make_pair(name.data(), index)) == s_stringMap.end())
    {
        return nullptr;
    }
    return s_stringMap.at(std::make_pair(name.data(), index)).c_str();
}

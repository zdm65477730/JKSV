#include "data/TitleInfo.hpp"

#include "colors.hpp"
#include "config.hpp"
#include "error.hpp"
#include "gfxutil.hpp"
#include "logger.hpp"
#include "stringutil.hpp"

#include <cstring>

data::TitleInfo::TitleInfo(uint64_t applicationID)
    : m_applicationID{applicationID}
    , m_data{std::make_unique<NsApplicationControlData>()}
{
    static constexpr size_t SIZE_CTRL_DATA = sizeof(NsApplicationControlData);
    static constexpr size_t SIZE_NACP      = sizeof(NacpStruct);

    uint64_t controlSize{};
    NacpLanguageEntry *entry{};
    NsApplicationControlData *data = m_data.get();

    // This will filter from even trying to fetch control data for system titles.
    const bool isSystem   = applicationID & 0x8000000000000000;
    const bool getError   = !isSystem && error::libnx(nsGetApplicationControlData(NsApplicationControlSource_Storage,
                                                                                m_applicationID,
                                                                                data,
                                                                                SIZE_CTRL_DATA,
                                                                                &controlSize));
    const bool entryError = !isSystem && !getError && error::libnx(nacpGetLanguageEntry(&data->nacp, &entry));
    if (isSystem || getError)
    {
        const std::string appIDHex = stringutil::get_formatted_string("%04X", m_applicationID & 0xFFFF);
        char *name                 = data->nacp.lang[SetLanguage_ENUS].name; // I'm hoping this is enough?

        std::memset(data, 0x00, SIZE_CTRL_DATA);
        std::snprintf(name, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID);
        m_icon = gfxutil::create_generic_icon(appIDHex, 48, colors::DIALOG_DARK, colors::WHITE);
        TitleInfo::get_create_path_safe_title();
    }
    else if (!getError && !entryError)
    {
        const size_t iconSize = controlSize - SIZE_NACP;
        m_hasData             = true;

        TitleInfo::get_create_path_safe_title();
        m_icon = sdl::TextureManager::create_load_texture(entry->name, data->icon, iconSize);
    }
}

// To do: Make this safer...
data::TitleInfo::TitleInfo(uint64_t applicationID, NsApplicationControlData &controlData)
    : m_applicationID{applicationID}
    , m_data{std::make_unique<NsApplicationControlData>()}
{
    NsApplicationControlData *data = m_data.get();

    std::memcpy(data, &controlData, sizeof(NsApplicationControlData));

    NacpLanguageEntry *entry{};
    const bool entryError = error::libnx(nacpGetLanguageEntry(&data->nacp, &entry));
    if (entryError) { std::snprintf(entry->name, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID); }

    TitleInfo::get_create_path_safe_title();
    m_icon = sdl::TextureManager::create_load_texture(entry->name, m_data->icon, sizeof(m_data->icon));
}

data::TitleInfo::TitleInfo(data::TitleInfo &&titleInfo) { *this = std::move(titleInfo); }

data::TitleInfo &data::TitleInfo::operator=(data::TitleInfo &&titleInfo)
{
    m_applicationID = titleInfo.m_applicationID;
    m_data          = std::move(titleInfo.m_data);
    m_hasData       = titleInfo.m_hasData;
    std::memcpy(m_pathSafeTitle, titleInfo.m_pathSafeTitle, TitleInfo::SIZE_PATH_SAFE);
    m_icon = titleInfo.m_icon;

    titleInfo.m_applicationID = 0;
    titleInfo.m_data          = nullptr;
    titleInfo.m_hasData       = false;
    std::memset(titleInfo.m_pathSafeTitle, 0x00, TitleInfo::SIZE_PATH_SAFE);
    titleInfo.m_icon = nullptr;

    return *this;
}

uint64_t data::TitleInfo::get_application_id() const { return m_applicationID; }

NsApplicationControlData *data::TitleInfo::get_control_data() { return m_data.get(); }

bool data::TitleInfo::has_control_data() const { return m_hasData; }

const char *data::TitleInfo::get_title()
{
    NacpLanguageEntry *entry{};
    const bool entryError = error::libnx(nacpGetLanguageEntry(&m_data->nacp, &entry));
    if (entryError) { return nullptr; }
    return entry->name;
}

const char *data::TitleInfo::get_path_safe_title() const { return m_pathSafeTitle; }

const char *data::TitleInfo::get_publisher()
{
    NacpLanguageEntry *entry{};
    const bool entryError = error::libnx(nacpGetLanguageEntry(&m_data->nacp, &entry));
    if (entryError) { return nullptr; }
    return entry->author;
}

uint64_t data::TitleInfo::get_save_data_owner_id() const { return m_data->nacp.save_data_owner_id; }

int64_t data::TitleInfo::get_save_data_size(uint8_t saveType) const
{
    const NacpStruct &nacp = m_data->nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account:   return nacp.user_account_save_data_size;
        case FsSaveDataType_Bcat:      return nacp.bcat_delivery_cache_storage_size;
        case FsSaveDataType_Device:    return nacp.device_save_data_size;
        case FsSaveDataType_Temporary: return nacp.temporary_storage_size;
        case FsSaveDataType_Cache:     return nacp.cache_storage_size;
    }
    return 0;
}

int64_t data::TitleInfo::get_save_data_size_max(uint8_t saveType) const
{
    const NacpStruct &nacp = m_data->nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account:   return std::max(nacp.user_account_save_data_size, nacp.user_account_save_data_size_max);
        case FsSaveDataType_Bcat:      return nacp.bcat_delivery_cache_storage_size;
        case FsSaveDataType_Device:    return std::max(nacp.device_save_data_size, nacp.device_save_data_size_max);
        case FsSaveDataType_Temporary: return nacp.temporary_storage_size;
        case FsSaveDataType_Cache:     return std::max(nacp.cache_storage_size, nacp.cache_storage_data_and_journal_size_max);
    }
    return 0;
}

int64_t data::TitleInfo::get_journal_size(uint8_t saveType) const
{
    const NacpStruct &nacp = m_data->nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account:   return nacp.user_account_save_data_journal_size;
        case FsSaveDataType_Bcat:      return nacp.bcat_delivery_cache_storage_size;
        case FsSaveDataType_Device:    return nacp.device_save_data_journal_size;
        case FsSaveDataType_Temporary: return nacp.temporary_storage_size;
        case FsSaveDataType_Cache:     return nacp.cache_storage_journal_size;
    }
    return 0;
}

int64_t data::TitleInfo::get_journal_size_max(uint8_t saveType) const
{
    const NacpStruct &nacp = m_data->nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account:
            return std::max(nacp.user_account_save_data_journal_size, nacp.user_account_save_data_journal_size_max);
        case FsSaveDataType_Bcat:      return nacp.bcat_delivery_cache_storage_size;
        case FsSaveDataType_Device:    return std::max(nacp.device_save_data_journal_size, nacp.device_save_data_journal_size_max);
        case FsSaveDataType_Temporary: return nacp.temporary_storage_size;
        case FsSaveDataType_Cache:
            return std::max(nacp.cache_storage_journal_size, nacp.cache_storage_data_and_journal_size_max);
    }
    return 0;
}

bool data::TitleInfo::has_save_data_type(uint8_t saveType) const
{
    const NacpStruct &nacp = m_data->nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account: return nacp.user_account_save_data_size > 0 || nacp.user_account_save_data_size_max > 0;
        case FsSaveDataType_Bcat:    return nacp.bcat_delivery_cache_storage_size > 0;
        case FsSaveDataType_Device:  return nacp.device_save_data_size > 0 || nacp.device_save_data_size_max > 0;
        case FsSaveDataType_Cache:   return nacp.cache_storage_size > 0 || nacp.cache_storage_data_and_journal_size_max > 0;
    }
    return false;
}

sdl::SharedTexture data::TitleInfo::get_icon() const { return m_icon; }

void data::TitleInfo::set_path_safe_title(const char *newPathSafe)
{
    const size_t length = std::char_traits<char>::length(newPathSafe);
    if (length >= TitleInfo::SIZE_PATH_SAFE) { return; }

    std::memset(m_pathSafeTitle, 0x00, TitleInfo::SIZE_PATH_SAFE);
    std::memcpy(m_pathSafeTitle, newPathSafe, length);
}

void data::TitleInfo::get_create_path_safe_title()
{
    const uint64_t applicationID = TitleInfo::get_application_id();
    NacpLanguageEntry *entry{};

    const bool hasCustom = config::has_custom_path(applicationID);
    if (hasCustom)
    {
        config::get_custom_path(applicationID, m_pathSafeTitle, TitleInfo::SIZE_PATH_SAFE);
        return;
    }

    const bool useTitleId = config::get_by_key(config::keys::USE_TITLE_IDS);
    const bool entryError = !useTitleId && error::libnx(nacpGetLanguageEntry(&m_data->nacp, &entry));
    const bool sanitized =
        !useTitleId && !entryError && stringutil::sanitize_string_for_path(entry->name, m_pathSafeTitle, SIZE_PATH_SAFE);
    if (useTitleId || entryError || !sanitized)
    {
        std::snprintf(m_pathSafeTitle, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID);
    }
}

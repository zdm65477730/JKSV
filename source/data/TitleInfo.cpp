#include "data/TitleInfo.hpp"

#include "config/config.hpp"
#include "error.hpp"
#include "graphics/colors.hpp"
#include "graphics/gfxutil.hpp"
#include "logging/logger.hpp"
#include "stringutil.hpp"

#include <cstring>

data::TitleInfo::TitleInfo(uint64_t applicationID) noexcept
    : m_applicationID(applicationID)
{
    static constexpr size_t SIZE_CTRL_DATA = sizeof(NsApplicationControlData);

    uint64_t controlSize{};

    // This will filter from even trying to fetch control data for system titles.
    const bool isSystem   = applicationID & 0x8000000000000000;
    const bool getError   = !isSystem && error::libnx(nsGetApplicationControlData(NsApplicationControlSource_Storage,
                                                                                m_applicationID,
                                                                                &m_data,
                                                                                SIZE_CTRL_DATA,
                                                                                &controlSize));
    const bool entryError = !getError && error::libnx(nacpGetLanguageEntry(&m_data.nacp, &m_entry));
    if (isSystem || getError)
    {
        const std::string appIDHex = stringutil::get_formatted_string("%04X", m_applicationID & 0xFFFF);
        char *name                 = m_data.nacp.lang[SetLanguage_ENUS].name; // I'm hoping this is enough?

        std::snprintf(name, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID);
        TitleInfo::get_create_path_safe_title();
    }
    else if (!getError && !entryError)
    {
        m_hasData = true;
        TitleInfo::get_create_path_safe_title();
    }
}

// To do: Make this safer...
data::TitleInfo::TitleInfo(uint64_t applicationID, NsApplicationControlData &controlData) noexcept
    : m_applicationID(applicationID)
{
    m_hasData = true;
    m_data    = controlData;

    const bool entryError = error::libnx(nacpGetLanguageEntry(&m_data.nacp, &m_entry));
    if (entryError) { std::snprintf(m_entry->name, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID); }

    TitleInfo::get_create_path_safe_title();
}

uint64_t data::TitleInfo::get_application_id() const noexcept { return m_applicationID; }

const NsApplicationControlData *data::TitleInfo::get_control_data() const noexcept { return &m_data; }

bool data::TitleInfo::has_control_data() const noexcept { return m_hasData; }

const char *data::TitleInfo::get_title() const noexcept { return m_entry->name; }

const char *data::TitleInfo::get_path_safe_title() const noexcept { return m_pathSafeTitle; }

const char *data::TitleInfo::get_publisher() const noexcept { return m_entry->author; }

uint64_t data::TitleInfo::get_save_data_owner_id() const noexcept { return m_data.nacp.save_data_owner_id; }

int64_t data::TitleInfo::get_save_data_size(uint8_t saveType) const noexcept
{
    const NacpStruct &nacp = m_data.nacp;
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

int64_t data::TitleInfo::get_save_data_size_max(uint8_t saveType) const noexcept
{
    const NacpStruct &nacp = m_data.nacp;
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

int64_t data::TitleInfo::get_journal_size(uint8_t saveType) const noexcept
{
    const NacpStruct &nacp = m_data.nacp;
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

int64_t data::TitleInfo::get_journal_size_max(uint8_t saveType) const noexcept
{
    const NacpStruct &nacp = m_data.nacp;
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

bool data::TitleInfo::has_save_data_type(uint8_t saveType) const noexcept
{
    const NacpStruct &nacp = m_data.nacp;
    switch (saveType)
    {
        case FsSaveDataType_Account: return nacp.user_account_save_data_size > 0 || nacp.user_account_save_data_size_max > 0;
        case FsSaveDataType_Bcat:    return nacp.bcat_delivery_cache_storage_size > 0;
        case FsSaveDataType_Device:  return nacp.device_save_data_size > 0 || nacp.device_save_data_size_max > 0;
        case FsSaveDataType_Cache:   return nacp.cache_storage_size > 0 || nacp.cache_storage_data_and_journal_size_max > 0;
    }
    return false;
}

sdl::SharedTexture data::TitleInfo::get_icon() const noexcept { return m_icon; }

void data::TitleInfo::set_path_safe_title(const char *newPathSafe) noexcept
{
    const size_t length = std::char_traits<char>::length(newPathSafe);
    if (length >= TitleInfo::SIZE_PATH_SAFE) { return; }

    std::memset(m_pathSafeTitle, 0x00, TitleInfo::SIZE_PATH_SAFE);
    std::memcpy(m_pathSafeTitle, newPathSafe, length);
}

void data::TitleInfo::get_create_path_safe_title() noexcept
{
    const uint64_t applicationID = TitleInfo::get_application_id();

    const bool hasCustom = config::has_custom_path(applicationID);
    if (hasCustom)
    {
        config::get_custom_path(applicationID, m_pathSafeTitle, TitleInfo::SIZE_PATH_SAFE);
        return;
    }

    const bool useTitleId = config::get_by_key(config::keys::USE_TITLE_IDS);
    const bool sanitized  = !useTitleId && stringutil::sanitize_string_for_path(m_entry->name, m_pathSafeTitle, SIZE_PATH_SAFE);
    if (useTitleId || !sanitized) { std::snprintf(m_pathSafeTitle, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID); }
}

void data::TitleInfo::load_icon()
{
    // This is taken from the NacpStruct.
    static constexpr size_t SIZE_ICON = 0x20000;

    if (m_hasData)
    {
        const std::string textureName = stringutil::get_formatted_string("%016llX", m_applicationID);
        m_icon                        = sdl::TextureManager::load(textureName, m_data.icon, SIZE_ICON);
    }
    else
    {
        const std::string text = stringutil::get_formatted_string("%04X", m_applicationID & 0xFFFF);
        m_icon                 = gfxutil::create_generic_icon(text, 48, colors::DIALOG_DARK, colors::WHITE);
    }
}

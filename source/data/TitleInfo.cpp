#include "data/TitleInfo.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "gfxutil.hpp"
#include "logger.hpp"
#include "stringutil.hpp"
#include <cstring>

data::TitleInfo::TitleInfo(uint64_t applicationID) : m_applicationID(applicationID)
{
    // Used to calculate icon size.
    uint64_t nsAppControlSize = 0;
    // Language entry
    NacpLanguageEntry *languageEntry = nullptr;

    Result nsError = nsGetApplicationControlData(NsApplicationControlSource_Storage,
                                                 applicationID,
                                                 &m_data,
                                                 sizeof(NsApplicationControlData),
                                                 &nsAppControlSize);

    if (R_FAILED(nsError) || nsAppControlSize < sizeof(m_data.nacp))
    {
        // This should be false by default, but just in case.
        m_hasData = false;

        // This is the lowest four hex values of the title.
        std::string applicationIDHex = stringutil::get_formatted_string("%04X", m_applicationID & 0xFFFF);

        // Blank the nacp just to be sure.
        std::memset(&m_data, 0x00, sizeof(NsApplicationControlData));

        // Sprintf title ids to language entries for safety.
        snprintf(m_data.nacp.lang[SetLanguage_ENUS].name, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID);

        // Path safe version of title.
        TitleInfo::get_create_path_safe_title();

        // Create the placeholder icon.
        m_icon = gfxutil::create_generic_icon(applicationIDHex, 48, colors::DIALOG_BOX, colors::WHITE);
    }
    else if (R_SUCCEEDED(nsError) && R_SUCCEEDED(nacpGetLanguageEntry(&m_data.nacp, &languageEntry)))
    {
        // Make sure title knows it has a valid control data struct.
        m_hasData = true;

        // Get a path safe version of the title.
        TitleInfo::get_create_path_safe_title();

        // Load the icon.
        m_icon = sdl::TextureManager::create_load_texture(languageEntry->name,
                                                          m_data.icon,
                                                          nsAppControlSize - sizeof(NacpStruct));
    }
}

// To do: Make this safer...
data::TitleInfo::TitleInfo(uint64_t applicationID, NsApplicationControlData &controlData)
    : m_applicationID(applicationID)
{
    // Start by making a copy of this.
    std::memcpy(&m_data, &controlData, sizeof(NsApplicationControlData));

    // Grab the language entry for the texture name.
    NacpLanguageEntry *entry = nullptr;
    if (R_FAILED(nacpGetLanguageEntry(&m_data.nacp, &entry)))
    {
        // sprintf the title ID to it. This buffer is the same as the path safe buffer so I'm using that.
        std::snprintf(entry->name, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID);
    }

    // Oops. Need this.
    TitleInfo::get_create_path_safe_title();

    // Load the icon to a texture. We're going to be lazy with the size here since it works anyway.
    m_icon = sdl::TextureManager::create_load_texture(entry->name, m_data.icon, sizeof(m_data.icon));
}

uint64_t data::TitleInfo::get_application_id(void) const
{
    return m_applicationID;
}

NsApplicationControlData *data::TitleInfo::get_control_data(void)
{
    return &m_data;
}

bool data::TitleInfo::has_control_data(void) const
{
    return m_hasData;
}

const char *data::TitleInfo::get_title(void)
{
    NacpLanguageEntry *entry = nullptr;
    if (R_FAILED(nacpGetLanguageEntry(&m_data.nacp, &entry)))
    {
        return nullptr;
    }
    return entry->name;
}

const char *data::TitleInfo::get_path_safe_title(void)
{
    return m_pathSafeTitle;
}

const char *data::TitleInfo::get_publisher(void)
{
    NacpLanguageEntry *Entry = nullptr;
    if (R_FAILED(nacpGetLanguageEntry(&m_data.nacp, &Entry)))
    {
        return nullptr;
    }
    return Entry->author;
}

uint64_t data::TitleInfo::get_save_data_owner_id(void) const
{
    return m_data.nacp.save_data_owner_id;
}

int64_t data::TitleInfo::get_save_data_size(uint8_t saveType) const
{
    // Create pointer to NACP since I don't feel like typing too much.
    const NacpStruct *nacp = &m_data.nacp;

    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            return nacp->user_account_save_data_size;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            return nacp->bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            return nacp->device_save_data_size;
        }
        break;

        case FsSaveDataType_Temporary:
        {
            return nacp->temporary_storage_size;
        }
        break;

        case FsSaveDataType_Cache:
        {
            return nacp->cache_storage_size;
        }
        break;

        default:
        {
            return 0;
        }
        break;
    }
    return 0;
}

int64_t data::TitleInfo::get_save_data_size_max(uint8_t saveType) const
{
    const NacpStruct *nacp = &m_data.nacp;

    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            return nacp->user_account_save_data_size_max > nacp->user_account_save_data_size
                       ? nacp->user_account_save_data_size_max
                       : nacp->user_account_save_data_size;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            return nacp->bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            return nacp->device_save_data_size_max > nacp->device_save_data_size ? nacp->device_save_data_size_max
                                                                                 : nacp->device_save_data_size;
        }
        break;

        case FsSaveDataType_Temporary:
        {
            return nacp->temporary_storage_size;
        }
        break;

        case FsSaveDataType_Cache:
        {
            return nacp->cache_storage_data_and_journal_size_max > nacp->cache_storage_size
                       ? nacp->cache_storage_data_and_journal_size_max
                       : nacp->cache_storage_size;
        }
        break;

        default:
        {
            return 0;
        }
        break;
    }
    return 0;
}

int64_t data::TitleInfo::get_journal_size(uint8_t saveType) const
{
    const NacpStruct *nacp = &m_data.nacp;

    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            return nacp->user_account_save_data_journal_size;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            // I'm just assuming this is right...
            return nacp->bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            return nacp->device_save_data_journal_size;
        }
        break;

        case FsSaveDataType_Temporary:
        {
            // Again, just assuming.
            return nacp->temporary_storage_size;
        }
        break;

        case FsSaveDataType_Cache:
        {
            return nacp->cache_storage_journal_size;
        }
        break;

        default:
        {
            return 0;
        }
        break;
    }
    return 0;
}

int64_t data::TitleInfo::get_journal_size_max(uint8_t saveType) const
{
    const NacpStruct *nacp = &m_data.nacp;

    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            return nacp->user_account_save_data_journal_size_max > nacp->user_account_save_data_journal_size
                       ? nacp->user_account_save_data_journal_size_max
                       : nacp->user_account_save_data_journal_size;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            return nacp->bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            return nacp->device_save_data_journal_size_max > nacp->device_save_data_journal_size
                       ? nacp->device_save_data_journal_size_max
                       : nacp->device_save_data_journal_size;
        }
        break;

        case FsSaveDataType_Temporary:
        {
            return nacp->temporary_storage_size;
        }
        break;

        case FsSaveDataType_Cache:
        {
            return nacp->cache_storage_data_and_journal_size_max > nacp->cache_storage_journal_size
                       ? nacp->cache_storage_data_and_journal_size_max
                       : nacp->cache_storage_journal_size;
        }
        break;

        default:
        {
            return 0;
        }
        break;
    }
    return 0;
}

bool data::TitleInfo::has_save_data_type(uint8_t saveType)
{
    NacpStruct *nacp = &m_data.nacp;

    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            return nacp->user_account_save_data_size > 0 || nacp->user_account_save_data_size_max > 0;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            return nacp->bcat_delivery_cache_storage_size > 0;
        }
        break;

        case FsSaveDataType_Device:
        {
            return nacp->device_save_data_size > 0 || nacp->device_save_data_size_max > 0;
        }
        break;

        case FsSaveDataType_Cache:
        {
            return nacp->cache_storage_size > 0 || nacp->cache_storage_data_and_journal_size_max > 0;
        }
        break;

        default:
        {
            return false;
        }
        break;
    }
    return false;
}

sdl::SharedTexture data::TitleInfo::get_icon(void) const
{
    return m_icon;
}

void data::TitleInfo::set_path_safe_title(const char *newPathSafe, size_t newPathLength)
{
    if (newPathLength >= TitleInfo::SIZE_PATH_SAFE)
    {
        return;
    }

    // Need to memset just in case the new title is shorter than the old one.
    std::memset(m_pathSafeTitle, 0x00, TitleInfo::SIZE_PATH_SAFE);
    std::memcpy(m_pathSafeTitle, newPathSafe, newPathLength);
}

void data::TitleInfo::get_create_path_safe_title(void)
{
    // Avoid calling this function over and over.
    uint64_t applicationID = TitleInfo::get_application_id();

    // Attempt to grab the language entry.
    NacpLanguageEntry *entry = nullptr;

    // If it has a custom path defined, use that. Else, try to make it path safe.
    if (config::has_custom_path(applicationID))
    {
        config::get_custom_path(applicationID, m_pathSafeTitle, TitleInfo::SIZE_PATH_SAFE);
    }
    else if (R_FAILED(nacpGetLanguageEntry(&m_data.nacp, &entry)) ||
             !stringutil::sanitize_string_for_path(entry->name, m_pathSafeTitle, TitleInfo::SIZE_PATH_SAFE))
    {
        std::snprintf(m_pathSafeTitle, TitleInfo::SIZE_PATH_SAFE, "%016lX", m_applicationID);
    }
}

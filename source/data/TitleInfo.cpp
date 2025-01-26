#include "data/TitleInfo.hpp"
#include "colors.hpp"
#include "logger.hpp"
#include "stringUtil.hpp"
#include <cstring>

data::TitleInfo::TitleInfo(uint64_t applicationID) : m_applicationID(applicationID)
{
    // Used to calculate icon size.
    uint64_t nsAppControlSize = 0;
    // Actual control data.
    NsApplicationControlData nsControlData;
    // Language entry
    NacpLanguageEntry *languageEntry = nullptr;

    Result nsError = nsGetApplicationControlData(NsApplicationControlSource_Storage,
                                                 applicationID,
                                                 &nsControlData,
                                                 sizeof(NsApplicationControlData),
                                                 &nsAppControlSize);

    if (R_FAILED(nsError) || nsAppControlSize < sizeof(nsControlData.nacp))
    {
        std::string applicationIDHex = stringutil::getFormattedString("%04X", applicationID & 0xFFFF);

        // Blank the nacp just to be sure.
        std::memset(&m_nacp, 0x00, sizeof(NacpStruct));

        // Sprintf title ids to language entries for safety.
        snprintf(m_nacp.lang[SetLanguage_ENUS].name, 0x200, "%016lX", applicationID);
        snprintf(m_pathSafeTitle, 0x200, "%016lX", applicationID);

        // Create a place holder icon.
        int textX = 128 - (sdl::text::getWidth(48, applicationIDHex.c_str()) / 2);
        m_icon = sdl::TextureManager::createLoadTexture(applicationIDHex, 256, 256, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
        m_icon->clear(colors::DIALOG_BOX);
        sdl::text::render(m_icon->get(), textX, 104, 48, sdl::text::NO_TEXT_WRAP, colors::WHITE, applicationIDHex.c_str());
    }
    else if (R_SUCCEEDED(nsError) && R_SUCCEEDED(nacpGetLanguageEntry(&nsControlData.nacp, &languageEntry)))
    {
        // Memcpy the NACP since it has all the good stuff.
        std::memcpy(&m_nacp, &nsControlData.nacp, sizeof(NacpStruct));
        // Get a path safe version of the title.
        if (!stringutil::sanitizeStringForPath(languageEntry->name, m_pathSafeTitle, 0x200))
        {
            std::sprintf(m_pathSafeTitle, "%016lX", applicationID);
        }
        // Load the icon.
        m_icon = sdl::TextureManager::createLoadTexture(languageEntry->name, nsControlData.icon, nsAppControlSize - sizeof(NacpStruct));
    }
}

uint64_t data::TitleInfo::getApplicationID(void) const
{
    return m_applicationID;
}

const char *data::TitleInfo::getTitle(void)
{
    NacpLanguageEntry *entry = nullptr;
    if (R_FAILED(nacpGetLanguageEntry(&m_nacp, &entry)))
    {
        return nullptr;
    }
    return entry->name;
}

const char *data::TitleInfo::getPathSafeTitle(void)
{
    return m_pathSafeTitle;
}

const char *data::TitleInfo::getPublisher(void)
{
    NacpLanguageEntry *Entry = nullptr;
    if (R_FAILED(nacpGetLanguageEntry(&m_nacp, &Entry)))
    {
        return nullptr;
    }
    return Entry->author;
}

uint64_t data::TitleInfo::getSaveDataOwnerID(void) const
{
    return m_nacp.save_data_owner_id;
}

int64_t data::TitleInfo::getSaveDataSize(FsSaveDataType saveType) const
{
    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            return m_nacp.user_account_save_data_size;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            return m_nacp.bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            return m_nacp.device_save_data_size;
        }
        break;

        case FsSaveDataType_Temporary:
        {
            return m_nacp.temporary_storage_size;
        }
        break;

        case FsSaveDataType_Cache:
        {
            return m_nacp.cache_storage_size;
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

int64_t data::TitleInfo::getSaveDataSizeMax(FsSaveDataType saveType) const
{
    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            return m_nacp.user_account_save_data_size_max > m_nacp.user_account_save_data_size ? m_nacp.user_account_save_data_size_max
                                                                                               : m_nacp.user_account_save_data_size;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            return m_nacp.bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            return m_nacp.device_save_data_size_max > m_nacp.device_save_data_size ? m_nacp.device_save_data_size_max
                                                                                   : m_nacp.device_save_data_size;
        }
        break;

        case FsSaveDataType_Temporary:
        {
            return m_nacp.temporary_storage_size;
        }
        break;

        case FsSaveDataType_Cache:
        {
            return m_nacp.cache_storage_data_and_journal_size_max > m_nacp.cache_storage_size ? m_nacp.cache_storage_data_and_journal_size_max
                                                                                              : m_nacp.cache_storage_size;
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

int64_t data::TitleInfo::getJournalSize(FsSaveDataType saveType) const
{
    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            return m_nacp.user_account_save_data_journal_size;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            // I'm just assuming this is right...
            return m_nacp.bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            return m_nacp.device_save_data_journal_size;
        }
        break;

        case FsSaveDataType_Temporary:
        {
            // Again, just assuming.
            return m_nacp.temporary_storage_size;
        }
        break;

        case FsSaveDataType_Cache:
        {
            return m_nacp.cache_storage_journal_size;
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

int64_t data::TitleInfo::getJournalSizeMax(FsSaveDataType saveType) const
{
    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            return m_nacp.user_account_save_data_journal_size_max > m_nacp.user_account_save_data_journal_size
                       ? m_nacp.user_account_save_data_journal_size_max
                       : m_nacp.user_account_save_data_journal_size;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            return m_nacp.bcat_delivery_cache_storage_size;
        }
        break;

        case FsSaveDataType_Device:
        {
            return m_nacp.device_save_data_journal_size_max > m_nacp.device_save_data_journal_size ? m_nacp.device_save_data_journal_size_max
                                                                                                   : m_nacp.device_save_data_journal_size;
        }
        break;

        case FsSaveDataType_Temporary:
        {
            return m_nacp.temporary_storage_size;
        }
        break;

        case FsSaveDataType_Cache:
        {
            return m_nacp.cache_storage_data_and_journal_size_max > m_nacp.cache_storage_journal_size
                       ? m_nacp.cache_storage_data_and_journal_size_max
                       : m_nacp.cache_storage_journal_size;
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

bool data::TitleInfo::hasSaveDataType(FsSaveDataType saveType)
{
    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            return m_nacp.user_account_save_data_size > 0 || m_nacp.user_account_save_data_size_max > 0;
        }
        break;

        case FsSaveDataType_Bcat:
        {
            return m_nacp.bcat_delivery_cache_storage_size > 0;
        }
        break;

        case FsSaveDataType_Device:
        {
            return m_nacp.device_save_data_size > 0 || m_nacp.device_save_data_size_max > 0;
        }
        break;

        case FsSaveDataType_Cache:
        {
            return m_nacp.cache_storage_size > 0 || m_nacp.cache_storage_data_and_journal_size_max > 0;
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

sdl::SharedTexture data::TitleInfo::getIcon(void) const
{
    return m_icon;
}

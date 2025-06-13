#include "fs/SaveMetaData.hpp"
#include "fs/directory_functions.hpp"
#include "fs/save_data_functions.hpp"
#include "fs/save_mount.hpp"
#include "fslib.hpp"
#include "logger.hpp"

namespace
{
    /// @brief This is the string template for errors here.
    constexpr std::string_view STRING_ERROR_TEMPLATE = "Error processing save meta for %016llX: %s";
} // namespace

void fs::create_save_meta_data(data::TitleInfo *titleInfo, const FsSaveDataInfo *saveInfo, fs::SaveMetaData &meta)
{
    // I'm assuming this is opened and good because we're making a backup.
    int64_t containerSize = 0;
    if (!fslib::get_device_total_space(fs::DEFAULT_SAVE_ROOT, containerSize))
    {
        // Log and fall back to file size count.
        logger::log("Error getting save container's total size. Defaulting to file size count.");
        containerSize = fs::get_directory_total_size(fs::DEFAULT_SAVE_ROOT);
    }

    // Fill out the meta struct.
    meta = {.m_magic = fs::SAVE_META_MAGIC,
            .m_applicationID = titleInfo->get_application_id(),
            .m_saveType = saveInfo->save_data_type,
            .m_saveRank = saveInfo->save_data_rank,
            .m_saveSpaceID = saveInfo->save_data_space_id,
            .m_saveDataSize = titleInfo->get_save_data_size(saveInfo->save_data_type),
            .m_saveDataSizeMax = titleInfo->get_journal_size_max(saveInfo->save_data_type),
            .m_journalSize = titleInfo->get_journal_size(saveInfo->save_data_type),
            .m_journalSizeMax = titleInfo->get_journal_size_max(saveInfo->save_data_type),
            .m_totalSaveSize = containerSize};
}

bool fs::process_save_meta_data(const FsSaveDataInfo *saveInfo, SaveMetaData &meta)
{
    if (meta.m_magic != SAVE_META_MAGIC || saveInfo->application_id != meta.m_applicationID)
    {
        logger::log(STRING_ERROR_TEMPLATE.data(), meta.m_applicationID, "Invalid magic or mismatched application ID.");
        return false;
    }

    // To do: I'm assuming this function will only be called once the save container is already opened here...
    int64_t totalSpace = 0;
    if (!fslib::get_device_total_space(fs::DEFAULT_SAVE_ROOT, totalSpace))
    {
        logger::log(STRING_ERROR_TEMPLATE.data(), meta.m_applicationID, "get_device_total_space");
        return false;
    }

    if (totalSpace >= meta.m_totalSaveSize)
    {
        // Gonna return true here, because there's no need to do anything to the container.
        return true;
    }

    // First we need to temporarily close the save.
    if (!fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT))
    {
        logger::log(STRING_ERROR_TEMPLATE.data(), meta.m_applicationID, "close_file_system");
        // We can't go any further at this point. You can't extend the save while it's open.
        return false;
    }

    // This is where we finally extend the container. Using the large of the two journal sizes
    if (!fs::extend_save_data(saveInfo, meta.m_totalSaveSize, meta.m_journalSizeMax))
    {
        logger::log(STRING_ERROR_TEMPLATE.data(), meta.m_applicationID, "Error extending save data container.");
        return false;
    }

    // If this fails, we're super screwed.
    if (!fslib::open_save_data_with_save_info(fs::DEFAULT_SAVE_MOUNT, *saveInfo))
    {
        logger::log(STRING_ERROR_TEMPLATE.data(), meta.m_applicationID, "open_save_data");
        return false;
    }

    // More later if needed.
    return true;
}

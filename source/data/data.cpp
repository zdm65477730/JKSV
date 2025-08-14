#include "data/data.hpp"

#include "appstates/DataLoadingState.hpp"
#include "appstates/FadeState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <switch.h>
#include <thread>
#include <unordered_map>
#include <vector>

namespace
{
    /// @brief This contains the user accounts on the system.
    std::vector<data::User> s_users{};

    // Map of Title info paired with its title/application
    std::unordered_map<uint64_t, data::TitleInfo> s_titleinfo{};

    /// @brief This vector holds pointers to everything that needs icons and processes it all on the main thread at the end.
    std::vector<data::DataCommon *> s_iconQueue;

    /// @brief Path used for cacheing title information since NS got slow on 20.0+
    constexpr std::string_view PATH_CACHE_PATH = "sdmc:/config/JKSV/cache.zip";

    // These are the ID's used for system type users.
    constexpr AccountUid ID_SYSTEM_USER = {FsSaveDataType_System};
    constexpr AccountUid ID_BCAT_USER   = {FsSaveDataType_Bcat};
    constexpr AccountUid ID_DEVICE_USER = {FsSaveDataType_Device};
    constexpr AccountUid ID_CACHE_USER  = {FsSaveDataType_Cache};

    /// @brief This is for loading the cache.
    constexpr size_t SIZE_CTRL_DATA = sizeof(NsApplicationControlData);
    constexpr size_t SIZE_SAVE_INFO = sizeof(FsSaveDataInfo);
} // namespace

/// @brief The main routine for the task to load data.
static void data_initialize_task(sys::Task *task, bool clearCache);

/// @brief Loads users from the system and creates the system users.
static bool load_create_user_accounts(sys::Task *task);

/// @brief Loads the application records from NS.
static void load_application_records(sys::Task *task);

/// @brief Imports external SVI(Control Data) files.
static void import_svi_files(sys::Task *task);

/// @brief Attempts to read the cache file from the SD.
/// @return True on success. False on failure.
static bool read_cache_file(sys::Task *task);

/// @brief Creates the cache file on the SD card.
static void create_cache_file();

/// @brief Processes the queues.
static void process_queue();

void data::launch_initialization(bool clearCache, std::function<void()> onDestruction)
{
    auto loadingState = DataLoadingState::create(data_initialize_task, clearCache);
    loadingState->add_destruct_function(process_queue);
    loadingState->add_destruct_function(create_cache_file);
    loadingState->add_destruct_function(onDestruction);
    StateManager::push_state(loadingState);
}

void data::get_users(data::UserList &userList)
{
    for (data::User &user : s_users) { userList.push_back(&user); }
}

data::TitleInfo *data::get_title_info_by_id(uint64_t applicationID)
{
    auto findTitle = s_titleinfo.find(applicationID);
    if (findTitle == s_titleinfo.end()) { return nullptr; }
    return &findTitle->second;
}

void data::load_title_to_map(uint64_t applicationID) { s_titleinfo.emplace(applicationID, applicationID); }

bool data::title_exists_in_map(uint64_t applicationID) { return s_titleinfo.find(applicationID) != s_titleinfo.end(); }

void data::get_title_info_list(data::TitleInfoList &listOut)
{
    for (auto &[applicationID, titleInfo] : s_titleinfo) { listOut.push_back(&titleInfo); }
}

void data::get_title_info_by_type(FsSaveDataType saveType, data::TitleInfoList &listOut)
{
    for (auto &[applicationID, titleInfo] : s_titleinfo)
    {
        if (titleInfo.has_save_data_type(saveType)) { listOut.push_back(&titleInfo); }
    }
}

static void data_initialize_task(sys::Task *task, bool clearCache)
{
    if (error::is_null(task)) { return; }
    const char *statusLoadingUserInfo = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 5);
    const char *statusFinalizing      = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 6);

    const fslib::Path &cachePath{PATH_CACHE_PATH};
    bool cacheExists = fslib::file_exists(cachePath);
    if (clearCache && cacheExists)
    {
        error::fslib(fslib::delete_file(cachePath));
        cacheExists = false;
    }

    if (s_users.empty()) { load_create_user_accounts(task); }

    const bool cacheRead = cacheExists && read_cache_file(task);
    if (!cacheRead)
    {
        s_titleinfo.clear();
        load_application_records(task);
        import_svi_files(task);
    }

    {
        for (data::User &user : s_users)
        {
            {
                const char *nickname     = user.get_nickname();
                const std::string status = stringutil::get_formatted_string(statusLoadingUserInfo, nickname);
                task->set_status(status);
            }
            user.load_user_data();
        }
    }

    task->set_status(statusFinalizing);
    for (data::User &user : s_users) { s_iconQueue.push_back(&user); }
    for (auto &[applicationID, titleInfo] : s_titleinfo) { s_iconQueue.push_back(&titleInfo); }
    task->complete();
}

static bool load_create_user_accounts(sys::Task *task)
{
    static constexpr const char *systemSafe = "System";
    static constexpr const char *bcatSafe   = "BCAT";
    static constexpr const char *deviceSafe = "Device";
    static constexpr const char *cacheSafe  = "Cache";

    if (error::is_null(task)) { return false; }

    const char *statusLoading  = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 0);
    const char *statusCreating = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 1);
    const char *systemName     = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 0);
    const char *bcatName       = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 2);
    const char *deviceName     = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 3);
    const char *cacheName      = strings::get_by_name(strings::names::SAVE_DATA_TYPES, 5);

    task->set_status(statusLoading);

    int total{};
    AccountUid accounts[8]{};
    const bool accountError = error::libnx(accountListAllUsers(accounts, 8, &total));
    const bool noAccounts   = total <= 0;
    if (accountError || noAccounts) { return false; }

    for (int i = 0; i < total; i++) { s_users.emplace_back(accounts[i], FsSaveDataType_Account); }

    task->set_status(statusCreating);
    s_users.emplace_back(ID_DEVICE_USER, deviceName, deviceSafe, FsSaveDataType_Device);
    s_users.emplace_back(ID_BCAT_USER, bcatName, bcatSafe, FsSaveDataType_Bcat);
    s_users.emplace_back(ID_CACHE_USER, cacheName, cacheSafe, FsSaveDataType_Cache);
    s_users.emplace_back(ID_SYSTEM_USER, systemName, systemSafe, FsSaveDataType_System);

    return true;
}

static void load_application_records(sys::Task *task)
{
    if (error::is_null(task)) { return; }

    int offset{};
    int count{};
    NsApplicationRecord record{};
    const char *statusLoadingRecords = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 2);

    bool listError{};
    do {
        listError = error::libnx(nsListApplicationRecord(&record, 1, offset++, &count)) || count <= 0;
        if (listError) { break; }

        {
            const std::string status = stringutil::get_formatted_string(statusLoadingRecords, record.application_id);
            task->set_status(status);
        }

        s_titleinfo.emplace(record.application_id, record.application_id);
    } while (!listError);
}

static void import_svi_files(sys::Task *task)
{
    static constexpr size_t SIZE_UINT64    = sizeof(uint64_t);
    static constexpr size_t SIZE_CTRL_DATA = sizeof(NsApplicationControlData);
    static constexpr size_t SIZE_SVI       = SIZE_UINT64 + SIZE_CTRL_DATA;

    if (error::is_null(task)) { return; }

    const fslib::Path sviPath = config::get_working_directory() / "svi";
    const fslib::Directory sviDir{sviPath};
    if (error::fslib(sviDir.is_open())) { return; }

    const char *statusLoadingSvi = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 3);
    task->set_status(statusLoadingSvi);

    const int64_t sviCount = sviDir.get_count();
    for (int64_t i = 0; i < sviCount; i++)
    {
        const fslib::Path target = sviPath / sviDir[i];
        fslib::File sviFile{target, FsOpenMode_Read};

        const bool goodFile = sviFile.is_open() && sviFile.get_size() == SIZE_SVI;
        if (!goodFile) { continue; }

        uint64_t applicationID{};
        auto controlData        = std::make_unique<NsApplicationControlData>();
        const bool idReadGood   = sviFile.read(&applicationID, SIZE_UINT64) == SIZE_UINT64;
        const bool dataReadGood = sviFile.read(controlData.get(), SIZE_CTRL_DATA) == SIZE_CTRL_DATA;
        if (!idReadGood || !dataReadGood) { continue; }

        data::TitleInfo newTitle{applicationID, controlData};
        s_titleinfo.emplace(applicationID, std::move(newTitle));
    }
}

static bool read_cache_file(sys::Task *task)
{
    if (error::is_null(task)) { return false; }

    fs::MiniUnzip cacheZip{PATH_CACHE_PATH};
    if (!cacheZip.is_open()) { return false; }

    const char *statusLoadingCache = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 4);
    task->set_status(statusLoadingCache);

    do {
        auto controlBuffer = std::make_unique<NsApplicationControlData>();
        const bool read    = cacheZip.read(controlBuffer.get(), SIZE_CTRL_DATA) == SIZE_CTRL_DATA;
        if (!read) { continue; }

        std::string_view filename{cacheZip.get_filename()};
        const size_t nameBegin = filename.find_first_not_of('/');
        if (nameBegin == filename.npos) { continue; } // This is required in order to get the application ID.

        filename                     = filename.substr(nameBegin);
        const uint64_t applicationID = std::strtoull(filename.data(), nullptr, 16);

        data::TitleInfo newTitle{applicationID, controlBuffer};
        s_titleinfo.emplace(applicationID, std::move(newTitle));
    } while (cacheZip.next_file());

    return true;
}

static void create_cache_file()
{
    const fslib::Path cachePath{PATH_CACHE_PATH};
    const bool cacheExists = fslib::file_exists(cachePath);
    if (cacheExists) { return; }

    fs::MiniZip cacheZip{PATH_CACHE_PATH};
    if (!cacheZip.is_open()) { return; }

    for (auto &[applicationID, titleInfo] : s_titleinfo)
    {
        if (!titleInfo.has_control_data()) { continue; }

        const NsApplicationControlData *controlData = titleInfo.get_control_data();
        const std::string cacheName                 = stringutil::get_formatted_string("//%016llX", applicationID);
        const bool opened                           = cacheZip.open_new_file(cacheName);
        if (!opened) { continue; }

        const bool controlWritten = cacheZip.write(controlData, SIZE_CTRL_DATA);
        if (!controlWritten) { logger::log("Error writing control data to zip!"); }
        cacheZip.close_current_file();
    }
}

static void process_queue()
{
    for (data::DataCommon *dataCommon : s_iconQueue) { dataCommon->load_icon(); }
    s_iconQueue.clear();
}

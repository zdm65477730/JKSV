#include "tasks/update.hpp"

#include "JKSV.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/MainMenuState.hpp"
#include "builddate.hpp"
#include "curl/curl.hpp"
#include "error.hpp"
#include "json.hpp"
#include "logging/logger.hpp"
#include "remote/remote.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"

#include <string_view>

// Defined at bottom.
static std::string get_git_json(curl::Handle &handle);
static bool get_date_as_ints(std::string_view dateString, int &month, int &day, int &year);

void tasks::update::check_for_update(sys::threadpool::JobData jobData)
{
    auto castData                = std::static_pointer_cast<MainMenuState::DataStruct>(jobData);
    MainMenuState *spawningState = castData->spawningState;

    // Just borrow this.
    if (!remote::has_internet_connection()) { return; }

    curl::Handle curlHandle   = curl::new_handle();
    const std::string gitJson = get_git_json(curlHandle);
    json::Object parser       = json::new_object(json_tokener_parse, gitJson.c_str());
    if (!parser) { return; }

    json_object *tagName = json::get_object(parser, "tag_name");
    if (!tagName) { return; }

    int month{}, day{}, year{};
    const char *dateString = json_object_get_string(tagName);
    get_date_as_ints(dateString, month, day, year);

    const bool greaterYear     = year > builddate::YEAR;
    const bool greaterMonth    = year == builddate::YEAR && month > builddate::MONTH;
    const bool greaterDay      = year == builddate::YEAR && month == builddate::MONTH && day > builddate::DAY;
    const bool updateAvailable = greaterYear || greaterMonth || greaterDay;
    if (updateAvailable) { spawningState->signal_update_found(); }
}

void tasks::update::download_update(sys::threadpool::JobData jobData)
{
    auto castData           = std::static_pointer_cast<sys::Task::DataStruct>(jobData);
    sys::ProgressTask *task = static_cast<sys::ProgressTask *>(castData->task);
    if (error::is_null(task)) { return; }

    curl::Handle downloadCurl = curl::new_handle();
    const std::string gitJson = get_git_json(downloadCurl);
    json::Object parser       = json::new_object(json_tokener_parse, gitJson.c_str());
    if (!parser) { TASK_FINISH_RETURN(task); }

    const char *downloadingFormat = strings::get_by_name(strings::names::IO_STATUSES, 4);
    std::string status            = stringutil::get_formatted_string(downloadingFormat, "JKSV.nro");
    task->set_status(status);

    json_object *assets = json::get_object(parser, "assets");
    if (!assets) { TASK_FINISH_RETURN(task); }

    json_object *assetZero = json_object_array_get_idx(assets, 0);
    if (!assetZero) { TASK_FINISH_RETURN(task); }

    json_object *downloadUrl  = json_object_object_get(assetZero, "browser_download_url");
    json_object *downloadSize = json_object_object_get(assetZero, "size");
    if (!downloadUrl || !downloadSize) { TASK_FINISH_RETURN(task); }

    const char *nroUrl     = json_object_get_string(downloadUrl);
    const uint64_t nroSize = json_object_get_uint64(downloadSize);
    task->reset(static_cast<double>(nroSize));

    // To do: Figure out how to get the argument from main here.
    error::libnx(romfsExit()); // This is needed so I can overwrite the NRO.
    fslib::File jksv{"sdmc:/switch/JKSV.nro", FsOpenMode_Create | FsOpenMode_Write, static_cast<int64_t>(nroSize)};
    if (error::fslib(jksv.is_open())) { TASK_FINISH_RETURN(task); }

    auto download = curl::create_download_struct(jksv, task, nroSize);
    curl::set_option(downloadCurl, CURLOPT_URL, nroUrl);
    curl::set_option(downloadCurl, CURLOPT_WRITEFUNCTION, curl::download_file_threaded);
    curl::set_option(downloadCurl, CURLOPT_WRITEDATA, download.get());
    curl::set_option(downloadCurl, CURLOPT_FOLLOWLOCATION, 1L);

    sys::threadpool::push_job(curl::download_write_thread_function, download);
    if (!curl::perform(downloadCurl)) { TASK_FINISH_RETURN(task); }
    download->writeComplete.acquire();

    task->complete();
}

static std::string get_git_json(curl::Handle &handle)
{
    static constexpr const char *URL_GIT_API = "https://api.github.com/repos/J-D-K/JKSV/releases/latest";

    std::string response{};
    curl::prepare_get(handle);
    curl::set_option(handle, CURLOPT_URL, URL_GIT_API);
    curl::set_option(handle, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(handle, CURLOPT_WRITEDATA, &response);
    curl::set_option(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl::perform(handle);

    return response;
}

static bool get_date_as_ints(std::string_view dateString, int &month, int &day, int &year)
{
    size_t monthSlash = dateString.find_first_of('/');
    if (monthSlash == dateString.npos) { return false; }

    const std::string monthString{dateString.substr(0, monthSlash)};

    size_t daySlash = dateString.find_first_of('/', ++monthSlash);
    if (daySlash == dateString.npos) { return false; }

    const std::string dayString{dateString.substr(monthSlash, daySlash - monthSlash)};
    const std::string yearString{dateString.substr(++daySlash, dateString.npos)};

    month = std::strtol(monthString.c_str(), nullptr, 10);
    day   = std::strtol(dayString.c_str(), nullptr, 10);
    year  = std::strtol(yearString.c_str(), nullptr, 10);

    return true;
}

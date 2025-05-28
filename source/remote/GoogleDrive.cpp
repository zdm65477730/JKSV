#include "remote/GoogleDrive.hpp"
#include "logger.hpp"
#include "strings.hpp"
#include <algorithm>

namespace
{
    /// @brief Buffer size for snprintf'ing URLS.
    constexpr size_t SIZE_URL_BUFFER = 0x800;

    /// @brief Path the client_secret from Google should be located at.
    constexpr std::string_view PATH_CLIENT_SECRET_LOCATION = "sdmc:/config/JKSV/client_secret.json";

    /// @brief Header for json type content.
    constexpr std::string_view HEADER_CONTENT_TYPE_JSON = "Content-Type: application/json";
    /// @brief Header for URL encoded content.
    constexpr std::string_view HEADER_CONTENT_TYPE_URL = "Content-Type: application/x-www-form-urlencoded";
    /// @brief Authorization header string that the token is appended to.
    constexpr std::string_view HEADER_AUTH_BEARER = "Authorization: Bearer ";
    /// @brief This is the string used to locate the header containing the upload URL.
    constexpr std::string_view HEADER_UPLOAD_LOCATION = "location";

    /// @brief OAuth2 login for TV type devices.
    /// @note This has limited scopes available.
    constexpr std::string_view URL_OAUTH2_DEVICE_CODE = "https://oauth2.googleapis.com/device/code";
    /// @brief This is the OAuth2 token URL.
    constexpr std::string_view URL_OAUTH2_TOKEN_URL = "https://oauth2.googleapis.com/token";
    /// @brief This a the V2 about api endpoint so I can pull the root drive ID. V3 doesn't support this.
    constexpr std::string_view URL_DRIVE_ABOUT_API = "https://www.googleapis.com/drive/v2/about";
    /// @brief This is the main V3 drive API url.
    constexpr std::string_view URL_DRIVE_FILE_API = "https://www.googleapis.com/drive/v3/files";
    /// @brief This is the endpoint for starting uploads.
    constexpr std::string_view URL_DRIVE_UPLOAD_API = "https://www.googleapis.com/upload/drive/v3/files";

    /// @brief The drive file scope allowed with the sign in method I chose this time around.
    constexpr std::string_view PARAM_DRIVE_FILE_SCOPE = "https://www.googleapis.com/auth/drive.file";
    /// @brief Grant type string for device type (TV/etc).
    constexpr std::string_view PARAM_GRANT_TYPE_DEVICE = "urn:ietf:params:oauth:grant-type:device_code";
    /// @brief These are the default parameters used to get a drive listing. JKSV loads everything it possible can at once.
    constexpr std::string_view PARAM_DEFUALT_LIST_QUERY =
        "fields=nextPageToken,files(name,id,size,parents,mimeType)&orderBy=name_natural&pageSize=256&q=trashed=false";

    // JSON keys
    constexpr std::string_view JSON_KEY_ACCESS_TOKEN = "access_token";
    constexpr std::string_view JSON_KEY_CLIENT_ID = "client_id";
    constexpr std::string_view JSON_KEY_CLIENT_SECRET = "client_secret";
    constexpr std::string_view JSON_KEY_DEVICE_CODE = "device_code";
    constexpr std::string_view JSON_KEY_ERROR = "error";
    constexpr std::string_view JSON_KEY_EXPIRES_IN = "expires_in";
    constexpr std::string_view JSON_KEY_GRANT_TYPE = "grant_type";
    constexpr std::string_view JSON_KEY_ID = "id";
    constexpr std::string_view JSON_KEY_INSTALLED = "installed";
    constexpr std::string_view JSON_KEY_MIMETYPE = "mimeType";
    constexpr std::string_view JSON_KEY_NAME = "name";
    constexpr std::string_view JSON_KEY_NEXT_PAGE_TOKEN = "nextPageToken";
    constexpr std::string_view JSON_KEY_PARENTS = "parents";
    constexpr std::string_view JSON_KEY_REFRESH_TOKEN = "refresh_token";

    /// @brief Folder mimetype string.
    constexpr std::string_view MIME_TYPE_DIRECTORY = "application/vnd.google-apps.folder";
} // namespace

remote::GoogleDrive::GoogleDrive(void) : m_curl(curl::new_handle())
{
    // Load the json file.
    json::Object clientJson = json::new_object(json_object_from_file, PATH_CLIENT_SECRET_LOCATION.data());
    if (!clientJson)
    {
        logger::log("Error reading Google Drive configuration json!");
        return;
    }

    json_object *installed = json::get_object(clientJson, JSON_KEY_INSTALLED.data());
    if (!installed)
    {
        logger::log("Google Drive configuration is invalid!");
        return;
    }

    json_object *clientId = json_object_object_get(installed, JSON_KEY_CLIENT_ID.data());
    json_object *clientSecret = json_object_object_get(installed, JSON_KEY_CLIENT_SECRET.data());
    if (!clientId || !clientSecret)
    {
        logger::log("Google Drive configuration is missing data!");
        return;
    }
    // Grab them.
    m_clientId = json_object_get_string(clientId);
    m_clientSecret = json_object_get_string(clientSecret);

    json_object *refreshToken = json_object_object_get(installed, JSON_KEY_REFRESH_TOKEN.data());
    if (!refreshToken)
    {
        // Bail here so it doesn't appear to be initialized.
        return.
    }
    m_refreshToken = json_object_get_string(refreshToken);
}

void remote::GoogleDrive::change_directory(std::string_view id)
{
    remote::Storage::List::iterator targetDir;
    if (id == ".." && (targetDir = GoogleDrive::find_directory_by_id(m_parent)) != m_list.end())
    {
        // Set it to the parent of the current parent.
        m_parent = targetDir->get_parent_id();
    }
    else if ((targetDir = Storage::find_directory(name)) != m_list.end())
    {
        m_parent = targetDir->get_id();
    }
    else
    {
        logger::log("Error changing Google Drive directory: directory specified not found!");
    }
}

bool remote::GoogleDrive::create_directory(std::string_view name)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        logger::log("Error creating directory: Token is no longer valid and could not be refreshed.");
        return false;
    }

    // Headers.
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);
    curl::append_header(headers, HEADER_CONTENT_TYPE_JSON.data());

    // Post json.
    json::object postJson = json::new_object(json_object_new_object);
    json_object *directoryName = json_object_new_string(name.data());
    json_object *mimeType = json_object_new_string(MIME_TYPE_DIRECTORY.data());
    json::add_object(postJson, JSON_KEY_NAME.data(), directoryName);
    json::add_object(postJson, JSON_KEY_MIMETYPE.data(), mimeType);
    // Append the parent. Empty parent will default to root.
    if (!m_parent.empty())
    {
        // I don't understand why this is an array.
        json_object *parentArray = json_object_new_array();
        json_object *parentId = json_object_new_string(m_parent.c_str());
        json_object_array_add(parentArray, parentId);
        json::add_object(postJson, JSON_KEY_PARENTS.data(), parentArray);
    }

    // Response string.
    std::string response;
    // Curl post
    curl::prepare_post(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, URL_DRIVE_FILE_API.data());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);
    curl::set_option(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(postJson.get()));

    if (!curl::perform())
    {
        return false;
    }

    // Parse the response.
    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser)
    {
        logger::log("Error creating directory: Error parsing server response.");
        return false;
    }

    // This is all I really need from the response.
    json_object *id = json::get_object(responseParser, JSON_KEY_ID.data());
    if (!id)
    {
        // This doesn't really mean the request wasn't successful. It just means the new directory couldn't be appended
        // to the list...
        logger::log("Error creating directory: Error finding directory ID in response!");
        return false;
    }

    // Emplace the new item.
    m_list.emplace_back(name, json_object_get_string(id), m_parent, true);

    return true;
}

// This is basically the same thing as deleting a file on Google Drive.
bool remote::GoogleDrive::delete_directory(std::string_view id)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    // Header list.
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);

    // URL
    char urlBuffer[SIZE_URL_BUFFER] = {0};
    std::snprintf(urlBuffer, SIZE_URL_BUFFER, "%s/%s", URL_DRIVE_FILE_API.data(), id.data());

    // Response. This is only used to check for errors.
    std::string response;
    // Curl.
    curl::reset_handle(m_curl);
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, urlBuffer);
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);

    if (!curl::perform(m_curl))
    {
        return false;
    }

    // This is weird. No response means success?
    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (GoogleDrive::error_occured(responseParser))
    {
        return false;
    }

    // Now remove it from the local list.
    remote::Storage::List::iterator findDir = GoogleDrive::find_directory_by_id(id);
    if (findDir == m_list.end())
    {
        logger::log("Error deleting directory from Google Drive: directory could not be located locally for removal.");
        return false;
    }

    // Erase the directory locally and all of its children.
    m_list.erase(findDir);
    // Just gonna reuse the iterator. This is one hell of a while loop condition.
    while ((findDir = std::find_if(m_list.begin(), m_list.end(), [id](const remote::Item &item) {
                return item.get_parent_id() == id;
            })) != m_list.end())
    {
        m_list.erase(findDir);
    }

    return true;
}

bool remote::GoogleDrive::upload_file(const fslib::Path &source)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    // Make sure we can even open the file before proceeding any further.
    fslib::File sourceFile(source, FsOpenMode_Read);
    if (!sourceFile)
    {
        logger::log("Error uploading file: %s", fslib::get_error_string());
        return false;
    }

    // Headers
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);
    curl::append_header(headers, HEADER_CONTENT_TYPE_JSON.data());

    // URL
    char urlBuffer[SIZE_URL_BUFFER] = {0};
    std::snprintf(urlBuffer, SIZE_URL_BUFFER, "%s?uploadType=resumable", URL_DRIVE_UPLOAD_API.data());

    // Json to post.
    json::Object postJson = json::new_object(json_object_new_object);
    json_object *driveName = json_object_new_string(source.get_file_name().data());
    json::add_object(postJson, JSON_KEY_NAME.data(), driveName);
    // Append the parent.
    if (!m_parent.empty())
    {
        json_object *parentArray = json_object_new_array();
        json_object *parentId = json_object_new_string(m_parent.c_str());
        json_object_array_add(parentArray, parentId);
        json::add_object(postJson, JSON_KEY_PARENTS.data(), parentArray);
    }

    // This requires reading a header to get the upload location.
    curl::HeaderArray headerArray;
    // Curl post.
    curl::prepare_post(m_curl);
    curl::set_option(m_curl, CURL_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_HEADERFUNCTION, curl::write_header_array);
    curl::set_option(m_curl, CURLOPT_HEADERDATA, &headerArray);
    curl::set_option(m_curl, CURLOPT_URL, urlBuffer);
    curl::set_option(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(postJson.get()));

    if (!curl::perform(m_curl))
    {
        return false;
    }

    // Extract the location from the headers.
    std::string location;
    if (!curl::get_header_value(headerArray, HEADER_UPLOAD_LOCATION.data(), location))
    {
        logger::log("Error uploading file: Couldn't extract upload location from headers.");
        return false;
    }

    // Response string.
    std::string response;
    // This is the actual upload. This doesn't need the authentication header to work.
    curl::prepare_upload(m_curl);
    curl::set_option(m_curl, CURLOPT_URL, location.c_str());
    curl::set_option(m_curl, CURLOPT_READFUNCTION, curl::read_data_from_file);
    curl::set_option(m_curl, CURLOPT_READDATA, &sourceFile);
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);

    if (!curl::perform(m_curl))
    {
        return false;
    }

    // Parse response.
    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser)
    {
        logger::log("Error uploading file: response could not be parsed.");
        return false;
    }

    json_object *id = json::get_object(responseParser, JSON_KEY_ID.data());
    json_object *filename = json::get_object(responseParser, JSON_KEY_NAME.data());
    json_object *mimeType = json::get_object(responseParser, JSON_KEY_MIMETYPE.data());
    // All of these are needed for the emplace_back.
    if (!id || !filename || !mimeType)
    {
        logger::log("Error uploading file: server response is missing data required.");
        return false;
    }

    m_list.emplace_back(json_object_get_string(filename),
                        json_object_get_string(id),
                        m_parent,
                        std::strcmp(MIME_TYPE_DIRECTORY.data(), json_object_get_string(mimeType)) == 0);

    // Assumed everything is fine!
    return true;
}

bool remote::GoogleDrive::patch_file(std::string_view id, const fslib::Path &source)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    fslib::File sourceFile(source, FsOpenMode_Read);
    if (!sourceFile)
    {
        logger::log("Error patching file: %s", fslib::get_error_string());
        return false;
    }

    char urlBuffer[SIZE_URL_BUFFER] = {0};
    std::snprintf(urlBuffer, SIZE_URL_BUFFER, "%s/%s?uploadType=resumable", URL_DRIVE_UPLOAD_API.data(), id.data());

    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);

    // Response string.
    std::string response;
    // Header array.
    curl::HeaderArray headerArray;
    // Curl
    curl::reset_handle(m_curl);
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_HEADERFUNCTION, curl::write_header_array);
    curl::set_option(m_curl, CURLOPT_HEADERDATA, &headerArray);
    curl::set_option(m_curl, CURLOPT_URL, urlBuffer);
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);

    if (!curl::perform(m_curl))
    {
        return false;
    }

    // Get the location from the headers.
    std::string location;
    if (!curl::get_header_value(headerArray, HEADER_UPLOAD_LOCATION.data(), location))
    {
        logger::log("Error patching file: upload location not found in headers!");
        return false;
    }

    // Setup the curl upload.
    curl::prepare_upload();
    curl::set_option(m_curl, CURLOPT_URL, location.c_str());
    curl::set_option(m_curl, CURLOPT_READFUNCTION, curl::read_data_from_file);
    curl::set_option(m_curl, CURLOPT_READDATA, &sourceFile);

    if (!curl::perform(m_curl))
    {
        return false;
    }

    // Try to locate the file with the ID.
    remote::Storage::List::iterator findFile = GoogleDrive::find_file_by_id(id);
    if (findFile == m_list.end())
    {
        logger::log("Error patching file: file could not be located for updating data.");
        return false;
    }

    // Update the size according to the source file instead of wasting time and pulling it from Google.
    findFile->set_size(sourceFile.get_size());

    return true;
}

bool remote::GoogleDrive::delete_file(std::string_view id)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    // Headers
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);

    char urlBuffer[SIZE_URL_BUFFER] = {0};
    std::snprintf(urlBuffer, SIZE_URL_BUFFER, "%s/%s", URL_DRIVE_FILE_API.data(), id.data());

    std::string response;
    curl::reset_handle(m_curl);
    curl::set_option(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);

    if (!curl::perform(m_curl))
    {
        return false;
    }

    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (GoogleDrive::error_occured(responseParser))
    {
        return false;
    }

    // Find the file and remove it from the list if it's found.
    remote::Storage::List::iterator findFile = GoogleDrive::find_file_by_id(id);
    if (findFile == m_list.end())
    {
        logger::log("Error deleting file from Drive: file could not be located in local list.");
        return false;
    }

    m_list.erase(findFile);

    return true;
}

bool remote::GoogleDrive::download_file(std::string_view id, const fslib::Path &destination)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    // Get iterator to file.
    remote::Storage::List::iterator findFile = GoogleDrive::find_file_by_id(id);
    if (findFile == m_list.end())
    {
        logger::log("Error downloading file: File information could not be located in local list!");
        return false;
    }

    // Try to open the file too before continuing.
    fslib::File destinationFile(destinationFile, FsOpenMode_Create | FsOpenMode_Write, findFile->get_size());
    if (!destinationFile)
    {
        logger::log("Error downloading file: local file could not be opened for writing!");
        return false;
    }

    // Headers
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, m_authHeader);

    // URL
    char urlBuffer[SIZE_URL_BUFFER] = {0};
    std::snprintf(urlBuffer, SIZE_URL_BUFFER, "%s/%s?alt=media", URL_DRIVE_FILE_API.data(), id.data());

    // Response string
    std::string response;
    // Curl
    curl::prepare_get(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, urlBuffer);
    // This is temporary until I get some stuff figured out.
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_data_to_file);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &destinationFile);

    if (!curl::perform(m_curl))
    {
        return false;
    }

    return true;
}

bool remote::GoogleDrive::sign_in_required(void) const
{
    return !m_isInitialized || m_refreshToken.empty();
}

bool remote::GoogleDrive::get_sign_in_data(std::string &signInString, std::time_t &expiration, int &waitInterval)
{
    // Header list.
    curl::HeaderList headers = curl::new_header_list();
    curl::append_header(headers, HEADER_CONTENT_TYPE_JSON.data());

    json::Object postJson = json::new_object(json_object_new_object);
    json_object *clientId = json_object_new_string(m_clientId.c_str());
    json_object *scope = json_object_new_string(PARAM_DRIVE_FILE_SCOPE.data());
    json::add_object(postJson, JSON_KEY_CLIENT_ID.data(), clientId);
    json::add_object(postJson, "scope", scope);

    std::string response;
    curl::prepare_get(m_curl);
    curl::set_option(m_curl, CURLOPT_HTTPHEADER, headers.get());
    curl::set_option(m_curl, CURLOPT_URL, URL_OAUTH2_DEVICE_CODE.data());
    curl::set_option(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl::set_option(m_curl, CURLOPT_WRITEDATA, &response);
    curl::set_option(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(postJson.get()));

    if (!curl::perform(m_curl))
    {
        return false;
    }

    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser || GoogleDrive::error_occurred(responseParser))
    {
        return false;
    }

    json_object *deviceCode = json::get_object(responseParser, JSON_KEY_DEVICE_CODE.data());
    json_object *userCode = json::get_object(responseParser, "user_code");
    json_object *verificationUrl = json::get_object(responseParser, "verification_url");
    json_object *expiresIn = json::get_object(responseParser, JSON_KEY_EXPIRES_IN.data());
    json_object *interval = json::get_object(responseParser, "interval");
    if (!deviceCode || !userCode || !verificationUrl || !expiresIn || !interval)
    {
        logger::log("Abnormal sign in response.");
        return false;
    }

    // Set expiration time.
    expiration = std::time(NULL) + json_object_get_uint64(expiresIn);
}

#include "remote/GoogleDrive.hpp"
#include "JSON.hpp"
#include "curl/curl.hpp"
#include "logger.hpp"
#include "remote/remote.hpp"
#include <cstring>

namespace
{
    // Header strings.
    // Content type.
    constexpr std::string_view HEADER_CONTENT_TYPE_APP_JSON = "Content-Type: application/json; charset=UTF-8";
    // Auth header format.
    constexpr std::string_view HEADER_AUTHORIZATION_BEARER = "Authorization: Bearer %s";

    // Mime type for folders.
    constexpr std::string_view MIME_TYPE_FOLDER = "application/vnd.google-apps.folder";

    // This is the URL format used to login with the Switch's web browser.
    constexpr std::string_view DRIVE_LOGIN_URL =
        "https://accounts.google.com/o/oauth2/v2/"
        "auth?client_id=%s&redirect_uri=urn:ietf:wg:oauth:2.0:oob:auto&response_type=code&scope=https:/"
        "/www.googleapis.com/auth/drive";
    // This is the oauth2 address for [when I remember how this works.]
    constexpr std::string_view DRIVE_TOKEN_URL = "https://oauth2.googleapis.com/token";
    // Need to figure this out.
    constexpr std::string_view DRIVE_APPROVAL_URL = "https://accounts.google.com/o/oauth2/approval";
    // The redirect uri
    constexpr std::string_view DRIVE_REDIRECT_URI = "urn:ietf:wg:oauth:2.0:oob:auto";
    // This is the URL format to ping to verify if a token is still valid to use.
    constexpr std::string_view DRIVE_TOKEN_VERIFY = "https://oauth2.googleapis.com/tokeninfo?%s";
    // This is the api URL to get listings?
    constexpr std::string_view DRIVE_FILE_API = "https://www.googleapis.com/drive/v3/files";
    // This is the api URL to start an upload.
    constexpr std::string_view DRIVE_UPLOAD_API = "https://www.googleapis.com/upload/drive/v3/files";

    // Grant string to get an authorization code.
    constexpr std::string_view GRANT_AUTH_CODE = "authorization_code";
    // Grant string to get a refresh token.
    constexpr std::string_view GRANT_REFRESH_TOKEN = "refresh_token";

    // JSON keys
    constexpr std::string_view JSON_KEY_INSTALLED = "installed";
    constexpr std::string_view JSON_KEY_CLIENT_ID = "client_id";
    constexpr std::string_view JSON_KEY_CLIENT_SECRET = "client_secret";
    constexpr std::string_view JSON_KEY_REFRESH_TOKEN = "refresh_token";
    constexpr std::string_view JSON_KEY_AUTH_CODE = "code";
    constexpr std::string_view JSON_KEY_REDIRECT_URI = "redirect_uri";
    constexpr std::string_view JSON_KEY_GRANT_TYPE = "grant_type";

    // This is the default query. I cache the entire drive listing in RAM to make everything smoother.
    // I don't like the hitches and pauses from downloading stuff as I go.
    constexpr std::string_view DEFAULT_QUERY = "?fields=files(name,id,mimeType,size,parents)&pageSize=1000&q=trashed="
                                               "false\%20and\%20\%27me\%27\%20in\%20owners";

    // This is the buffer size for snprintf'ing and formatting URL strings.
    constexpr int URL_BUFFER_SIZE = 0x300;
} // namespace

remote::GoogleDrive::GoogleDrive(std::string_view jsonPath)
{
    // New json_object wrapped in unique_ptr.
    json::Object driveJson = json::new_object(json_object_from_file, jsonPath.data());
    if (!driveJson)
    {
        return;
    }

    // Everything is wrapped in this.
    json_object *installed = json_object_object_get(driveJson.get(), JSON_KEY_INSTALLED.data());
    if (!installed)
    {
        return;
    }

    // Check for our client strings.
    json_object *client = json_object_object_get(driveJson.get(), JSON_KEY_CLIENT_ID.data());
    json_object *clientSecret = json_object_object_get(driveJson.get(), JSON_KEY_CLIENT_SECRET.data());
    if (!client || !clientSecret)
    {
        // Just bail because this isn't a valid client_secret file.
        return;
    }
    // Set the strings.
    m_clientId = json_object_get_string(client);
    m_clientSecret = json_object_get_string(clientSecret);

    // Check for a refresh token. If JKSV ran previously and was successful, we should have it here.
    json_object *refreshToken = json_object_object_get(driveJson.get(), JSON_KEY_REFRESH_TOKEN.data());
    if (refreshToken)
    {
        m_refreshToken = json_object_get_string(refreshToken);
    }

    // Init was successful. unique_ptr wrap should take care of json cleanup for us.
    m_isInitialized = true;
}

remote::GoogleDrive::~GoogleDrive()
{
    curl_slist_free_all(m_headers);
}

bool remote::GoogleDrive::create_directory(std::string_view directoryName, std::string_view parentId)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        // There's no real way to continue.
        return false;
    }

    // Construct json post.
    json::Object postJson = json::new_object(json_object_new_object);
    json_object *dirName = json_object_new_string(directoryName.data());
    json_object *mimeType = json_object_new_string(MIME_TYPE_FOLDER);
    json_object_object_add(postJson.get(), "name", dirName);
    json_object_object_add(postJson.get(), "mimeType", mimeType);
    // Append the parent if it's present.
    if (!parentId.empty())
    {
        // This is so stupid. Why is it an array if there's only one parent?
        json_object *parentArray = json_object_new_array();
        json_object *parent = json_object_new_string(parentId.data());
        json_object_array_add(parentArray, parent);
        json_object_object_add(postJson.get(), parentArray);
    }

    std::string response;
    Storage::curl_prepare_post();
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);
    curl_easy_setopt(m_curl, CURLOPT_URL, DRIVE_UPLOAD_API);
    curl_easy_setopt(m_curl, CURLTOP_POSTFIELDS, json_object_get_string(postJson.get()));
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);

    if (!Storage::curl_perform())
    {
        return false;
    }

    // Parse the response.
    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser || GoogleDrive::error_occured(responseParser.get()))
    {
        return false;
    }

    // I do this in memory instead of pulling a new listing so everything is smoother.
    json_object *directoryId = json_object_object_get(responseParser.get(), "id");
    // Gonna be safe.
    if (!directoryId)
    {
        logger::log("GoogleDrive: Error creating directory: No directory ID found in response!");
        return false;
    }

    // Just push a new directory using the id.
    m_remoteList.emplace(directoryName, json_object_get_string(directoryId), parentId, 0, true);

    return true;
}

bool remote::GoogleDrive::upload_file(std::string_view filename, std::string_view parentId, fslib::File &target)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    // Create the full upload API URL.
    char uploadUrl[URL_BUFFER_SIZE] = {0};
    std::snprintf(uploadUrl, URL_BUFFER_SIZE, "%s?uploadType=resumable", DRIVE_UPLOAD_API);

    // Json
    json::Object postJson = json::new_object(json_object_new_object);
    if (!postJson)
    {
        return false;
    }

    json_object *filenameString = json_object_new_string(filename.data());
    json_object_object_add(postJson.get(), "name", filenameString);
    if (!parentId.empty())
    {
        json_object *parentArray = json_object_new_array();
        json_object *parentIdString = json_object_new_string(parentId.data());
        json_object_array_add(parentArray, parentIdString);
        json_object_object_add(parentArray);
    }

    // Need the headers from this.
    curl::HeaderArray headers;

    // Prepare base post.
    Storage::curl_prepare_post();
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);
    curl_easy_setopt(m_curl, CURLOPT_URL, uploadUrl);
    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, curl::write_headers_array);
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &headers);
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(postJson.get()));

    if (!Storage::curl_perform())
    {
        return false;
    }

    std::string location;
    if (!curl::extract_value_from_headers(headers, "location", location))
    {
        logger::log("Error finding value in headers!\n");
        return false;
    }

    // Prepare to upload.
    std::string response;
    Storage::curl_prepare_upload();
    curl_easy_setopt(m_curl, CURLOPT_URL, location.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, curl::read_from_file);
    curl_easy_setopt(m_curl, CURLOPT_READDATA, &target);

    if (!Storage::curl_perform())
    {
        return false;
    }

    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser)
    {
        return false;
    }

    json_object *fileId = json_object_object_get(responseParser.get(), "id");
    json_object *name = json_object_object_get(responseParser.get(), "name");
    json_object *mimeType = json_object_object_get(responseParser.get(), "mimeType");
    if (!fileId || !name || !mimeType)
    {
        logger::log("Error uploading file: Invalid response received.");
        return false;
    }

    m_remoteList.emplace_back(json_object_get_string(fileId),
                              json_object_get_string(fileId),
                              parentId,
                              target.get_size(),
                              false);

    return true;
}

bool remote::GoogleDrive::patch_file(std::string_view fileId, fslib::File &target)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    char patchUrl[URL_BUFFER_SIZE] = {0};
    std::snprintf(patchUrl, URL_BUFFER_SIZE, "%s/%s>uploadType=resumable", DRIVE_UPLOAD_API, fileId.data());

    std::string response;
    curl::HeaderArray headers;
    Storage::curl_prepare_patch();
    curl_easy_setopt(m_curl, CURLOPT_URL, patchUrl);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, curl::write_headers_array);
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &headers);

    if (!Storage::curl_perform())
    {
        return false;
    }

    std::string location;
    if (!curl::extract_value_from_headers(headers, "Location", location))
    {
        return false;
    }

    Storage::curl_prepare_upload();
    curl_easy_setopt(m_curl, CURLOPT_URL, location.c_str());
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, curl::read_from_file);
    curl_easy_setopt(m_curl, CURLOPT_READDATA, &target);

    if (!Storage::curl_perform())
    {
        return false;
    }

    // Update the file size in the listing according to the file uploaded. No point in wasting time fetching it.
    auto findFile = std::find(m_remoteList.begin(), m_remoteList.end(), [fileId](const remote::StorageItem &item) {
        return item.get_id() == fileId;
    });
    if (findFile == m_remoteList.end())
    {
        return false;
    }

    findFile->set_file_size(target.get_size());

    return true;
}

bool remote::GoogleDrive::download_file(std::string_view fileId, fslib::File &target)
{
    if (!GoogleDrive::token_is_valid() && !GoogleDrive::refresh_token())
    {
        return false;
    }

    char downloadUrl[URL_BUFFER_SIZE] = {0};
    std::snprintf(downloadUrl, URL_BUFFER_SIZE, "%s/%s?alt=media", DRIVE_FILE_API, fileId.data());

    Storage::curl_prepare_get();
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, curl::write_headers_array);
    curl_easy_setopt(m_curl, CURLOPT_URL, downloadUrl);
}

bool remote::GoogleDrive::delete_file(std::string_view fileId)
{
}

bool remote::GoogleDrive::sign_in_and_authenticate(std::string &authCodeOut)
{
    // This shouldn't be able to happen, but just in case.
    if (m_isInitialized)
    {
        return;
    }

    // snprint the login url together.
    char loginUrl[URL_BUFFER_SIZE] = {0};
    std::snprintf(loginUrl, URL_BUFFER_SIZE, DRIVE_LOGIN_URL, m_clientId.c_str());

    // We use the reply struct to get the token from the last page.
    WebCommonConfig webConfig;
    WebCommonReply webReply;
    // Hard to read, but screw it. All web stuff in one shot.
    if (R_FAILED(webPageCreate(&webConfig, loginUrl)) ||
        R_FAILED(webConfigSetCallbackUrl(&webConfig, DRIVE_APPROVAL_URL)) ||
        R_FAILED(webConfigShow(&webConfig, &webReply)))
    {
        logger::log("Error occurred while configuring and launching the web applet");
        return false;
    }

    // Now we get the URL from the web reply and cut out the authentication code.
    size_t replyLength = 0;
    char replyUrl[URL_BUFFER_SIZE] = {0};
    if (R_FAILED(webReplyGetLastUrl(&webReply, replyUrl, URL_BUFFER_SIZE, &replyLength)))
    {
        logger::log("Error occurred while retrieving the last URL from web reply.");
        return false;
    }

    // Old JKSV used to do this with C++ strings, but here I'm gonna use C functions.
    // This should get us to the beginning of the approval code.
    char *approvalBegin = std::strstr(replyUrl, "approvalCode") + 13;
    if (!approvalBegin)
    {
        logger::log("Error reading approval code from web reply.");
        return false;
    }

    char *approvalEnd = std::strpbrk(replyUrl, "#&");
    if (!approvalEnd)
    {
        logger::log("Error finding the end of the approval code.");
        return false;
    }

    // This should set the authentication code to the substring in the web reply.
    authCodeOut.assign(approvalCode, approvalEnd - approvalBegin);

    return true;
}

bool remote::GoogleDrive::exchange_auth_code(std::string_view authCode)
{
    // Post json
    json::Object postJson = json::new_object(json_object_new_object);
    // Doing this with json-c is kinda annoying.
    json_object *clientIdString = json_object_new_string(m_clientId.c_str());
    json_object *clientSecretString = json_object_new_string(m_clientSecret.c_str());
    json_object *authCodeString = json_object_new_string(authCode.data());
    json_object *redirectUriString = json_object_new_string(DRIVE_REDIRECT_URI.data());
    json_object *grantTypeString = json_object_new_string(GRANT_AUTH_CODE.data());
    // Add it to the post json.
    json_object_object_add(postJson.get(), "client_id", clientIdString);
    json_object_object_add(postJson.get(), "client_secret", clientSecretString);
    json_object_object_add(postJson.get(), "code", authCode.data());
    json_object_object_add(postJson.get(), "redirect_uri", redirectUriString);
    json_object_object_add(postJson.get(), "grant_type", grantTypeString);

    // String for post.
    std::string response;
    // Curl post.
    Storage::curl_prepare_post();
    curl_easy_setopt(m_curl, CURLOPT_URL, DRIVE_TOKEN_URL.data());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(postJson.get()));

    if (!Storage::curl_perform())
    {
        return false;
    }

    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser || GoogleDrive::error_occured(responseParser.get()))
    {
        return false;
    }

    json_object *accessToken = json_object_object_get(responseParser.get());
    json_object *refreshToken = json_object_object_get(responseParser.get());
    if (!accessToken || !refreshToken)
    {
        logger::log("Error exchanging authentication code: Token and/or refresh token not found!");
        return false;
    }

    // Store them
    m_token = json_object_get_string(accessToken);
    m_refreshToken = json_object_get_string(refresh_token);

    // Done? Or are we?
    return true;
}

bool remote::GoogleDrive::refresh_token(void)
{
    // Post json.
    json::Object postJson = json::new_object(json_object_new_object);

    // Assemble~
    json_object *clientIdString = json_object_new_string(m_clientId.c_str());
    json_object *clientSecretString = json_object_new_string(m_clientSecret.c_str());
    json_object *refreshTokenString = json_object_new_string(m_refreshToken.c_str());
    json_object *grantTypeString = json_object_new_string(GRANT_REFRESH_TOKEN.data());
    json_object_object_add(postJson.get(), JSON_KEY_CLIENT_ID.data(), clientIdString);
    json_object_object_add(postJson.get(), JSON_KEY_CLIENT_SECRET.data(), clientSecretString);
    json_object_object_add(postJson.get(), JSON_KEY_REFRESH_TOKEN.data(), refreshTokenString);
    json_object_object_add(postJson.get(), JSON_KEY_GRANT_TYPE.data(), grantTypeString);

    // Prepare post.
    std::string response;
    Storage::curl_prepare_post();
    curl_easy_setopt(m_curl, CURLOPT_URL, DRIVE_TOKEN_URL.data());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, json_object_get_string(postJson.get()));

    if (!Storage::curl_perform())
    {
        return false;
    }

    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser || GoogleDrive::error_occured(responseParser.get()))
    {
        return false;
    }

    json_object *accessToken = json_object_object_get(postJson.get(), "access_token");
    if (!accessToken)
    {
        logger::log("Access token not found!");
        return false;
    }

    m_token = json_object_get_string(accessToken);

    return true;
}

bool remote::GoogleDrive::token_is_valid(void)
{
    char tokenVerifyUrl[URL_BUFFER_SIZE] = {0};
    std::snprintf(tokenVerifyUrl, URL_BUFFER_SIZE, "%saccess_token=%s", m_token.c_str());

    std::string response;
    Storage::curl_prepare_get();
    curl_easy_setopt(m_curl, CURLOPT_URL, tokenVerifyUrl);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);

    if (!Storage::curl_perform())
    {
        return false;
    }

    json::Object responseParser = json::new_object(json_tokener_parse, response.c_str());
    if (!responseParser || GoogleDrive::error_occured(responseParser.get()))
    {
        return false;
    }

    // I'm assuming no error found means everything is fine.
    return true;
}

bool remote::GoogleDrive::request_listing(const char *listingUrl, std::string &jsonOut)
{
    Storage::curl_prepare_get();
    curl_easy_setopt(m_curl, CURLOPT_URL, listingUrl);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl::write_response_string);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &jsonOut);

    return Storage::curl_perform();
}

bool remote::GoogleDrive::process_listing(std::string_view listingJson, bool clear)
{
    json::Object listParser = json::new_object(json_tokener_parse, listingJson.data());
    json_object *fileArray = nullptr;
    // Check both at once.
    if (!listParser || !(files = json_object_object_get(listParser.get(), "files")))
    {
        logger::log("Error parsing Google Drive listing!");
        return false;
    }

    size_t listCount = json_object_array_length(listParser.get());
    if (listCount == 0)
    {
        logger::log("Google Drive listing is empty!");
        return false;
    }

    // Clear and reserve if desired.
    if (clear)
    {
        m_remoteList.clear();
        m_remoteList.reserve(listCount);
    }

    for (size_t i = 0; i < listCount; i++)
    {
        json_object *currentListing = json_object_array_get_idx(fileArray, i);
        if (!currentListing)
        {
            logger::log("Error parsing drive listing. THIS SHOULDN'T BE ABLE TO HAPPEN!");
            break;
        }

        // Get our keys.
        json_object *idString = json_object_object_get(listParser.get(), "id");
        json_object *nameString = json_object_object_get(listParser.get(), "name");
        json_object *mimeTypeString = json_object_object_get(listParser.get(), "mimeType");
        json_object *sizeString = json_object_object_get(listParser.get(), "size");
        // I don't understand why this is a fucking array.
        json_object *parentArray = json_object_object_get(listParser.get(), "parents");
        json_object *parent = nullptr;

        // Parent is held in a array for some reason. This should work fine. Remember to test it, JK.
        if (parentArray)
        {
            size_t parentLength = json_object_array_length(parentArray);
            for (size_t i = 0; i < parentLength; i++)
            {
                // I'm hoping this works how I want. Google making this an array even though items can only really
                // have one parent is really stupid.
                parent = json_object_array_get_idx(parentArray, i);
            }
        }

        // Emplace
        m_remoteList.emplace_back(json_object_get_string(nameString),
                                  json_object_get_string(idString),
                                  json_object_get_string(parent),
                                  json_object_get_int64(sizeString),
                                  std::strcmp(MIME_TYPE_FOLDER.data(), json_object_get_string(mimeTypeString) == 0));
    }

    return true;
}

bool remote::GoogleDrive::error_occured(json_object *json)
{
    json_object *error = json_object_object_get(json, "error");
    if (error)
    {
        return true;
    }
    return false;
}

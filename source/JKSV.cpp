#include "JKSV.hpp"

#include "StateManager.hpp"
#include "appstates/FileModeState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TaskState.hpp"
#include "config/config.hpp"
#include "curl/curl.hpp"
#include "data/data.hpp"
#include "error.hpp"
#include "fslib.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "logging/logger.hpp"
#include "remote/remote.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"

#include <chrono>
#include <switch.h>
#include <thread>

// Normally I try to avoid C macros in C++, but this cleans stuff up nicely.
#define ABORT_ON_FAILURE(x)                                                                                                    \
    if (!x) { return; }

namespace
{
    /// @brief Build month.
    constexpr uint8_t BUILD_MON = 9;
    /// @brief Build day.
    constexpr uint8_t BUILD_DAY = 6;
    /// @brief Year.
    constexpr uint16_t BUILD_YEAR = 2025;

    /// @brief Config for socket.
    constexpr SocketInitConfig SOCKET_INIT_CONFIG = {.tcp_tx_buf_size     = 0x20000,
                                                     .tcp_rx_buf_size     = 0x20000,
                                                     .tcp_tx_buf_max_size = 0x80000,
                                                     .tcp_rx_buf_max_size = 0x80000,
                                                     .udp_tx_buf_size     = 0x2400,
                                                     .udp_rx_buf_size     = 0xA500,
                                                     .sb_efficiency       = 8,
                                                     .num_bsd_sessions    = 3,
                                                     .bsd_service_type    = BsdServiceType_User};

} // namespace

template <typename... Args>
static bool initialize_service(Result (*function)(Args...), const char *serviceName, Args... args)
{
    Result error = (*function)(args...);
    if (R_FAILED(error))
    {
        logger::log("Error initializing %s: 0x%X.", serviceName, error);
        return false;
    }
    return true;
}

// Definition at bottom.
static void finish_initialization();

// This can't really have an initializer list since it sets everything up.
JKSV::JKSV()
{
    appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad);
    ABORT_ON_FAILURE(JKSV::initialize_services());
    ABORT_ON_FAILURE(JKSV::initialize_filesystem());
    logger::initialize();
    ABORT_ON_FAILURE(JKSV::initialize_sdl());

    ABORT_ON_FAILURE(curl::initialize());
    input::initialize();
    config::initialize();

    ABORT_ON_FAILURE(strings::initialize()); // This is fatal now.

    const char *translationFormat = strings::get_by_name(strings::names::TRANSLATION, 0);
    const char *author            = strings::get_by_name(strings::names::TRANSLATION, 1);
    m_showTranslationInfo         = std::char_traits<char>::compare(author, "NULL", 4) != 0; // This is whether or not to show.
    m_translationInfo             = stringutil::get_formatted_string(translationFormat, author);
    m_buildString                 = stringutil::get_formatted_string("v. %02d.%02d.%04d", BUILD_MON, BUILD_DAY, BUILD_YEAR);

    // This needs the config init'd or read to work.
    JKSV::create_directories();

    data::launch_initialization(false, finish_initialization);

    m_isRunning = true;
}

JKSV::~JKSV()
{
    config::save();
    curl::exit();
    JKSV::exit_services();
    sdl::text::exit();
    sdl::exit();

    appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
}

bool JKSV::is_running() const noexcept { return m_isRunning && appletMainLoop(); }

void JKSV::update()
{
    input::update();

    const bool plusPressed = input::button_pressed(HidNpadButton_Plus);
    const bool isClosable  = StateManager::back_is_closable();
    if (plusPressed && isClosable) { m_isRunning = false; }

    StateManager::update();
    ui::PopMessageManager::update();
}

void JKSV::render()
{
    sdl::frame_begin(colors::CLEAR_COLOR);

    // Top and bottom divider lines.
    sdl::render_line(sdl::Texture::Null, 30, 88, 1250, 88, colors::WHITE);
    sdl::render_line(sdl::Texture::Null, 30, 648, 1250, 648, colors::WHITE);
    // Icon
    m_headerIcon->render(sdl::Texture::Null, 66, 27);
    // "JKSV"
    sdl::text::render(sdl::Texture::Null, 130, 32, 34, sdl::text::NO_WRAP, colors::WHITE, "JKSV");

    // Translation info in bottom left.
    if (m_showTranslationInfo)
    {
        sdl::text::render(sdl::Texture::Null, 8, 680, 14, sdl::text::NO_WRAP, colors::WHITE, m_translationInfo);
    }
    // Build date
    sdl::text::render(sdl::Texture::Null, 8, 700, 14, sdl::text::NO_WRAP, colors::WHITE, m_buildString);

    StateManager::render();
    ui::PopMessageManager::render();

    sdl::frame_end();
}

bool JKSV::initialize_filesystem()
{
    // This needs to be in this specific order
    const bool fslib    = fslib::is_initialized();
    const bool romfs    = initialize_service(romfsInit, "RomFS");
    const bool fslibDev = fslib && fslib::dev::initialize_sdmc();
    if (!fslib || !romfs || !fslibDev) { return false; }

    return true;
}

bool JKSV::initialize_services()
{
    // This looks cursed, but it works.
    bool serviceInit = initialize_service(accountInitialize, "Account", AccountServiceType_Administrator);
    serviceInit      = serviceInit && initialize_service(nsInitialize, "NS");
    serviceInit      = serviceInit && initialize_service(pdmqryInitialize, "PDMQry");
    serviceInit      = serviceInit && initialize_service(plInitialize, "PL", PlServiceType_User);
    serviceInit      = serviceInit && initialize_service(pmshellInitialize, "PMShell");
    serviceInit      = serviceInit && initialize_service(setInitialize, "Set");
    serviceInit      = serviceInit && initialize_service(setsysInitialize, "SetSys");
    serviceInit      = serviceInit && initialize_service(socketInitialize, "Socket", &SOCKET_INIT_CONFIG);
    serviceInit      = serviceInit && initialize_service(nifmInitialize, "NIFM", NifmServiceType_User);
    return serviceInit;
}

bool JKSV::initialize_sdl()
{
    bool sdlInit = sdl::initialize("JKSV", 1280, 720);
    sdlInit      = sdlInit && sdl::text::initialize();
    m_headerIcon = sdl::TextureManager::load("headerIcon", "romfs:/Textures/HeaderIcon.png");
    JKSV::add_color_chars();
    return sdlInit && m_headerIcon;
}

bool JKSV::create_directories()
{
    // Working directory creation.
    const fslib::Path workDir{config::get_working_directory()};
    const bool needsWorkDir   = !fslib::directory_exists(workDir);
    const bool workDirCreated = needsWorkDir && fslib::create_directories_recursively(workDir);
    if (needsWorkDir && !workDirCreated) { return false; }

    // Trash folder. This can fail without being fatal.
    const fslib::Path trashDir{workDir / "_TRASH_"};
    const bool trashEnabled = config::get_by_key(config::keys::ENABLE_TRASH_BIN);
    const bool needsTash    = trashEnabled && !fslib::directory_exists(trashDir);
    if (needsTash) { error::fslib(fslib::create_directory(trashDir)); }

    return true;
}

void JKSV::add_color_chars()
{
    sdl::text::add_color_character(L'#', colors::BLUE);
    sdl::text::add_color_character(L'*', colors::DARK_RED);
    sdl::text::add_color_character(L'<', colors::YELLOW);
    sdl::text::add_color_character(L'>', colors::GREEN);
    sdl::text::add_color_character(L'`', colors::BLUE_GREEN);
    sdl::text::add_color_character(L'^', colors::PINK);
}

void JKSV::exit_services()
{
    nifmExit();
    socketExit();
    setsysExit();
    setExit();
    pmshellExit();
    plExit();
    pdmqryExit();
    nsExit();
    accountExit();
}

static void finish_initialization()
{
    MainMenuState::create_and_push();

    if (fslib::file_exists(remote::PATH_GOOGLE_DRIVE_CONFIG)) { remote::initialize_google_drive(); }
    else if (fslib::file_exists(remote::PATH_WEBDAV_CONFIG)) { remote::initialize_webdav(); }
}

#include "JKSV.hpp"

#include "StateManager.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TaskState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "curl/curl.hpp"
#include "data/data.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "remote/remote.hpp"
#include "sdl.hpp"
#include "strings.hpp"
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
    constexpr uint8_t BUILD_MON = 7;
    /// @brief Build day.
    constexpr uint8_t BUILD_DAY = 28;
    /// @brief Year.
    constexpr uint16_t BUILD_YEAR = 2025;
} // namespace

template <typename... Args>
static bool initialize_service(Result (*function)(Args...), const char *serviceName, Args... args)
{
    Result error = (*function)(args...);
    if (R_FAILED(error))
    {
        logger::log("Error initializing %s: 0x%X.", error);
        return false;
    }
    return true;
}

// This can't really have an initializer list since it sets everything up.
JKSV::JKSV()
{
    const std::time_t beginTime = std::time(nullptr);

    appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad);
    ABORT_ON_FAILURE(JKSV::initialize_services());
    ABORT_ON_FAILURE(JKSV::initialize_filesystem());
    logger::initialize();
    ABORT_ON_FAILURE(JKSV::initialize_sdl());

    ABORT_ON_FAILURE(curl::initialize());
    ABORT_ON_FAILURE(strings::initialize()); // This is fatal now.

    const char *translationFormat = strings::get_by_name(strings::names::TRANSLATION, 0);
    const char *author            = strings::get_by_name(strings::names::TRANSLATION, 1);
    m_showTranslationInfo         = std::char_traits<char>::compare(author, "NULL", 4) != 0; // This is whether or not to show.
    m_translationInfo             = stringutil::get_formatted_string(translationFormat, author);
    m_buildString                 = stringutil::get_formatted_string("v. %02d.%02d.%04d", BUILD_MON, BUILD_DAY, BUILD_YEAR);

    input::initialize();
    config::initialize();

    // This needs the config init'd or read to work.
    JKSV::create_directories();

    // Data loading depends on the config being read or init'd.
    ABORT_ON_FAILURE(data::initialize(false));

    // Push initial main menu state.
    auto mainMenu = MainMenuState::create();
    StateManager::push_state(mainMenu);

    // Init drive or webdav.
    if (fslib::file_exists(remote::PATH_GOOGLE_DRIVE_CONFIG)) { remote::initialize_google_drive(); }
    else if (fslib::file_exists(remote::PATH_WEBDAV_CONFIG)) { remote::initialize_webdav(); }

    const std::time_t endTime = std::time(nullptr);
    const double diff         = std::difftime(endTime, beginTime);
    logger::log("Boot time: %.02f seconds.", diff);

    m_isRunning = true;
}

JKSV::~JKSV()
{
    config::save();
    curl::exit();
    JKSV::exit_services();
    sdl::text::exit();
    sdl::exit();
    fslib::exit();
    appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
}

bool JKSV::is_running() const { return m_isRunning; }

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
    const bool fslib    = fslib::initialize();
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
    serviceInit      = serviceInit && initialize_service(socketInitializeDefault, "Socket");
    serviceInit      = serviceInit && initialize_service(nifmInitialize, "NIFM", NifmServiceType_User);
    return serviceInit;
}

bool JKSV::initialize_sdl()
{
    bool sdlInit = sdl::initialize("JKSV", 1280, 720);
    sdlInit      = sdlInit && sdl::text::initialize();
    m_headerIcon = sdl::TextureManager::create_load_texture("headerIcon", "romfs:/Textures/HeaderIcon.png");
    JKSV::add_color_chars();
    return sdlInit && m_headerIcon;
}

bool JKSV::create_directories()
{
    // Working directory creation.
    const fslib::Path workDir = config::get_working_directory();
    const bool needsWorkDir   = !fslib::directory_exists(workDir);
    const bool workDirCreated = needsWorkDir && fslib::create_directories_recursively(workDir);
    if (needsWorkDir && !workDirCreated) { return false; }

    // SVI folder.
    const fslib::Path sviDir = workDir / "svi";
    const bool needsSviDir   = !fslib::directory_exists(sviDir);
    const bool sviDirCreated = needsSviDir && fslib::create_directory(sviDir);
    if (needsSviDir && !sviDirCreated) { return false; }

    return true;
}

void JKSV::add_color_chars()
{
    sdl::text::add_color_character(L'#', colors::BLUE);
    sdl::text::add_color_character(L'*', colors::RED);
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

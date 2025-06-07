#include "JKSV.hpp"
#include "appstates/MainMenuState.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "data/data.hpp"
#include "fslib.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "ui/PopMessageManager.hpp"
#include <switch.h>

// Normally I try to avoid C macros in C++, but this cleans stuff up nicely.
#define ABORT_ON_FAILURE(x)                                                                                            \
    if (!x)                                                                                                            \
    {                                                                                                                  \
        return;                                                                                                        \
    }

namespace
{
    /// @brief Build month.
    constexpr uint8_t BUILD_MON = 5;

    /// @brief Build day.
    constexpr uint8_t BUILD_DAY = 31;

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

JKSV::JKSV(void)
{
    // FsLib
    ABORT_ON_FAILURE(fslib::initialize());

    // This doesn't rely on stdio or anything.
    logger::initialize();

    // Need to init RomFS here for now until I update FsLib to take care of this. Never mind. That isn't going to happen.
    ABORT_ON_FAILURE(initialize_service(romfsInit, "RomFS"));

    // Let FsLib take care of calls to SDMC instead of fs_dev
    ABORT_ON_FAILURE(fslib::dev::initialize_sdmc());

    // SDL
    ABORT_ON_FAILURE(sdl::initialize("JKSV", 1280, 720));
    ABORT_ON_FAILURE(sdl::text::initialize());

    // Services.
    // Using administrator so JKSV can still run in Applet mode, barely.
    ABORT_ON_FAILURE(initialize_service(accountInitialize, "Account", AccountServiceType_Administrator));
    ABORT_ON_FAILURE(initialize_service(nsInitialize, "NS"));
    ABORT_ON_FAILURE(initialize_service(pdmqryInitialize, "PDMQry"));
    ABORT_ON_FAILURE(initialize_service(plInitialize, "PL", PlServiceType_User));
    ABORT_ON_FAILURE(initialize_service(pmshellInitialize, "PMShell"));
    ABORT_ON_FAILURE(initialize_service(setInitialize, "Set"));
    ABORT_ON_FAILURE(initialize_service(setsysInitialize, "SetSys"));
    ABORT_ON_FAILURE(initialize_service(socketInitializeDefault, "Socket"));

    // JKSV doesn't really need the full GPU going so.
    if (R_FAILED(appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad)))
    {
        logger::log("Error setting CPU boost mode!");
    }

    // Input doesn't have anything to return.
    input::initialize();

    // Neither does config.
    config::initialize();

    // Get and create working directory. There isn't much of an FS anymore.
    fslib::Path workingDirectory = config::get_working_directory();
    if (!fslib::directory_exists(workingDirectory) && !fslib::create_directories_recursively(workingDirectory))
    {
        logger::log("Error creating working directory: %s", fslib::get_error_string());
        return;
    }

    // JKSV also has no internal strings anymore. This is FATAL now.
    ABORT_ON_FAILURE(strings::initialize());

    if (!data::initialize(false))
    {
        return;
    }

    // Install/setup our color changing characters.
    sdl::text::add_color_character(L'#', colors::BLUE);
    sdl::text::add_color_character(L'*', colors::RED);
    sdl::text::add_color_character(L'<', colors::YELLOW);
    sdl::text::add_color_character(L'>', colors::GREEN);
    sdl::text::add_color_character(L'`', colors::BLUE_GREEN);
    sdl::text::add_color_character(L'^', colors::PINK);

    // This is to check whether the author wanted credit for their work.
    m_showTranslationInfo =
        std::char_traits<char>::compare(strings::get_by_name(strings::names::TRANSLATION_INFO, 1), "NULL", 4) != 0;

    // This can't be in an initializer list because it needs SDL initialized.
    m_headerIcon = sdl::TextureManager::create_load_texture("HeaderIcon", "romfs:/Textures/HeaderIcon.png");

    // Push initial main menu state.
    JKSV::push_state(std::make_shared<MainMenuState>());

    m_isRunning = true;
}

JKSV::~JKSV()
{
    // Try to save config first.
    config::save();

    // Not sure if this one is really needed, but just in case.
    appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
    socketExit();
    setsysExit();
    setExit();
    pmshellExit();
    plExit();
    pdmqryExit();
    nsExit();
    accountExit();
    sdl::text::exit();
    sdl::exit();
    fslib::exit();
}

bool JKSV::is_running(void) const
{
    return m_isRunning;
}

void JKSV::update(void)
{
    input::update();

    if (input::button_pressed(HidNpadButton_Plus) && !sm_stateVector.empty() && sm_stateVector.back()->is_closable())
    {
        m_isRunning = false;
    }

    JKSV::update_state_vector();

    // Update pop messages.
    ui::PopMessageManager::update();
}

void JKSV::render(void)
{
    sdl::frame_begin(colors::CLEAR_COLOR);
    // Top and bottom divider lines.
    sdl::render_line(NULL, 30, 88, 1250, 88, colors::WHITE);
    sdl::render_line(NULL, 30, 648, 1250, 648, colors::WHITE);
    // Icon
    m_headerIcon->render(NULL, 66, 27);
    // "JKSV"
    sdl::text::render(NULL, 130, 32, 34, sdl::text::NO_TEXT_WRAP, colors::WHITE, "JKSV");
    // Translation info in bottom left.
    if (m_showTranslationInfo)
    {
        sdl::text::render(NULL,
                          8,
                          680,
                          14,
                          sdl::text::NO_TEXT_WRAP,
                          colors::WHITE,
                          strings::get_by_name(strings::names::TRANSLATION_INFO, 0),
                          strings::get_by_name(strings::names::TRANSLATION_INFO, 1));
    }
    // Build date
    sdl::text::render(NULL,
                      8,
                      700,
                      14,
                      sdl::text::NO_TEXT_WRAP,
                      colors::WHITE,
                      "v. %02d.%02d.%04d",
                      BUILD_MON,
                      BUILD_DAY,
                      BUILD_YEAR);

    // State render loop.
    if (!sm_stateVector.empty())
    {
        for (auto &CurrentState : sm_stateVector)
        {
            CurrentState->render();
        }
    }

    // Render messages.
    ui::PopMessageManager::render();

    sdl::frame_end();
}

void JKSV::push_state(std::shared_ptr<AppState> newState)
{
    if (!sm_stateVector.empty())
    {
        sm_stateVector.back()->take_focus();
    }
    newState->give_focus();
    sm_stateVector.push_back(newState);
}

void JKSV::update_state_vector(void)
{
    if (sm_stateVector.empty())
    {
        return;
    }

    // Check for and purge deactivated states.
    for (size_t i = 0; i < sm_stateVector.size(); i++)
    {
        if (!sm_stateVector.at(i)->is_active())
        {
            // This is a just in case thing. Some states are never actually purged.
            sm_stateVector.at(i)->take_focus();
            sm_stateVector.erase(sm_stateVector.begin() + i);
        }
    }

    // Make sure the back has focus.
    if (!sm_stateVector.back()->has_focus())
    {
        sm_stateVector.back()->give_focus();
    }

    // Only update the back most state.
    sm_stateVector.back()->update();
}

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

#define ABORT_ON_FAILURE(x)                                                                                                                    \
    if (!x)                                                                                                                                    \
    {                                                                                                                                          \
        return;                                                                                                                                \
    }

namespace
{
    constexpr uint8_t BUILD_MON = 1;
    constexpr uint8_t BUILD_DAY = 6;
    constexpr uint16_t BUILD_YEAR = 2025;
} // namespace

template <typename... Args>
static bool initializeService(Result (*function)(Args...), const char *serviceName, Args... args)
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

    // Need to init RomFS here for now until I update FsLib to take care of this.
    ABORT_ON_FAILURE(initializeService(romfsInit, "RomFS"));

    // Let FsLib take care of calls to SDMC instead of fs_dev
    ABORT_ON_FAILURE(fslib::dev::initializeSDMC());

    // SDL
    ABORT_ON_FAILURE(sdl::initialize("JKSV", 1280, 720));
    ABORT_ON_FAILURE(sdl::text::initialize());

    // Services.
    // Using administrator so JKSV can still run in Applet mode.
    ABORT_ON_FAILURE(initializeService(accountInitialize, "Account", AccountServiceType_Administrator));
    ABORT_ON_FAILURE(initializeService(nsInitialize, "NS"));
    ABORT_ON_FAILURE(initializeService(pdmqryInitialize, "PDMQry"));
    ABORT_ON_FAILURE(initializeService(plInitialize, "PL", PlServiceType_User));
    ABORT_ON_FAILURE(initializeService(pmshellInitialize, "PMShell"));
    ABORT_ON_FAILURE(initializeService(setInitialize, "Set"));
    ABORT_ON_FAILURE(initializeService(setsysInitialize, "SetSys"));
    ABORT_ON_FAILURE(initializeService(socketInitializeDefault, "Socket"));

    // Input doesn't have anything to return.
    input::initialize();

    // Neither does config.
    config::initialize();

    // Get and create working directory. There isn't much of an FS anymore.
    fslib::Path workingDirectory = config::getWorkingDirectory();
    if (!fslib::directoryExists(workingDirectory) && !fslib::createDirectoriesRecursively(workingDirectory))
    {
        logger::log("Error creating working directory: %s", fslib::getErrorString());
        return;
    }

    // JKSV also has no internal strings anymore. This is FATAL now.
    ABORT_ON_FAILURE(strings::initialize());

    if (!data::initialize())
    {
        return;
    }

    // Install/setup our color changing characters.
    sdl::text::addColorCharacter(L'#', colors::BLUE);
    sdl::text::addColorCharacter(L'*', colors::RED);
    sdl::text::addColorCharacter(L'<', colors::YELLOW);
    sdl::text::addColorCharacter(L'>', colors::GREEN);
    sdl::text::addColorCharacter(L'^', colors::PINK);

    // This is to check whether the author wanted credit for their work.
    m_showTranslationInfo = std::char_traits<char>::compare(strings::getByName(strings::names::TRANSLATION_INFO, 1), "NULL", 4) != 0;

    // This can't be in an initializer list because it needs SDL initialized.
    m_headerIcon = sdl::TextureManager::createLoadTexture("HeaderIcon", "romfs:/Textures/HeaderIcon.png");

    // Push initial main menu state.
    JKSV::pushState(std::make_shared<MainMenuState>());

    m_isRunning = true;
}

JKSV::~JKSV()
{
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

bool JKSV::isRunning(void) const
{
    return m_isRunning;
}

void JKSV::update(void)
{
    input::update();

    if (input::buttonPressed(HidNpadButton_Plus) && !sm_stateVector.empty() && sm_stateVector.back()->isClosable())
    {
        m_isRunning = false;
    }

    if (!sm_stateVector.empty())
    {
        while (!sm_stateVector.back()->isActive())
        {
            sm_stateVector.back()->takeFocus();
            sm_stateVector.pop_back();
            sm_stateVector.back()->giveFocus();
        }
        sm_stateVector.back()->update();
    }

    // Update pop messages.
    ui::PopMessageManager::update();
}

void JKSV::render(void)
{
    sdl::frameBegin(colors::CLEAR_COLOR);
    // Top and bottom divider lines.
    sdl::renderLine(NULL, 30, 88, 1250, 88, colors::WHITE);
    sdl::renderLine(NULL, 30, 648, 1250, 648, colors::WHITE);
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
                          strings::getByName(strings::names::TRANSLATION_INFO, 0),
                          strings::getByName(strings::names::TRANSLATION_INFO, 1));
    }
    // Build date
    sdl::text::render(NULL, 8, 700, 14, sdl::text::NO_TEXT_WRAP, colors::WHITE, "v. %02d.%02d.%04d", BUILD_MON, BUILD_DAY, BUILD_YEAR);

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

    sdl::frameEnd();
}

void JKSV::pushState(std::shared_ptr<AppState> newState)
{
    if (!sm_stateVector.empty())
    {
        sm_stateVector.back()->takeFocus();
    }
    newState->giveFocus();
    sm_stateVector.push_back(newState);
}

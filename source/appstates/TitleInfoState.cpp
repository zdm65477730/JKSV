#include "appstates/TitleInfoState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings.hpp"
#include "stringutil.hpp"
#include <ctime>

namespace
{
    /// @brief This is the width of the panel in pixels.
    constexpr int SIZE_PANEL_WIDTH = 480;

    /// @brief This is the gap on both sides
    constexpr int SIZE_PANEL_SUB = 16;

    /// @brief This is the size used for rendering text here.
    constexpr int SIZE_FONT = 24;

    /// @brief This is the vertical size in pixels of the render targets for title/publisher.
    constexpr int SIZE_TEXT_TARGET_HEIGHT = 40;
} // namespace

TitleInfoState::TitleInfoState(data::User *user, data::TitleInfo *titleInfo)
    : m_user(user), m_titleInfo(titleInfo), m_icon(m_titleInfo->get_icon())
{
    // This needs to be checked. All title information panels share these members.
    if (!sm_initialized)
    {
        // This is the slide panel everything is rendered to.
        sm_slidePanel = std::make_unique<ui::SlideOutPanel>(SIZE_PANEL_WIDTH, ui::SlideOutPanel::Side::Right);

        // This is the render target for the title.
        sm_titleTarget = sdl::TextureManager::create_load_texture("infoTitleTarget",
                                                                  SIZE_PANEL_WIDTH - SIZE_PANEL_SUB,
                                                                  SIZE_TEXT_TARGET_HEIGHT,
                                                                  SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);

        // This is the render target for the publisher.
        sm_publisherTarget =
            sdl::TextureManager::create_load_texture("infoPublisherTarget",
                                                     SIZE_PANEL_WIDTH - SIZE_PANEL_SUB,
                                                     SIZE_TEXT_TARGET_HEIGHT,
                                                     SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
        sm_initialized = true;
    }

    // Do this instead of calling the function everytime.
    uint64_t applicationID = m_titleInfo->get_application_id();

    // These is needed at some point.
    FsSaveDataInfo *saveInfo = m_user->get_save_info_by_id(applicationID);
    PdmPlayStatistics *playStats = m_user->get_play_stats_by_id(applicationID);

    // Title text.
    m_titleScroll
        .create(m_titleInfo->get_title(), SIZE_FONT, SIZE_PANEL_WIDTH - SIZE_PANEL_SUB, 6, false, colors::WHITE);
    // Publisher.
    m_publisherScroll
        .create(m_titleInfo->get_publisher(), SIZE_FONT, SIZE_PANEL_WIDTH - SIZE_PANEL_SUB, 6, false, colors::WHITE);

    // Grab the application ID string.
    m_applicationID =
        stringutil::get_formatted_string(strings::get_by_name(strings::names::TITLE_INFO_STRINGS, 0), applicationID);
    // Same here. I think these use lower case characters on the NAND?
    m_saveDataID = stringutil::get_formatted_string(strings::get_by_name(strings::names::TITLE_INFO_STRINGS, 1),
                                                    saveInfo->save_data_id);
    // This is simple.
    m_totalLaunches = stringutil::get_formatted_string(strings::get_by_name(strings::names::TITLE_INFO_STRINGS, 5),
                                                       playStats->total_launches);
    // This should be semi-simple.
    m_saveDataType = stringutil::get_formatted_string(
        strings::get_by_name(strings::names::TITLE_INFO_STRINGS, 6),
        strings::get_by_name(strings::names::SAVE_DATA_TYPES, saveInfo->save_data_type));

    // Going to "cheat" with the next two. This works, so screw it.
    char playBuffer[0x40] = {0};
    std::tm *firstPlayed = std::localtime(reinterpret_cast<const time_t *>(&playStats->first_timestamp_user));
    std::strftime(playBuffer, 0x40, strings::get_by_name(strings::names::TITLE_INFO_STRINGS, 2), firstPlayed);
    m_firstPlayed.assign(playBuffer);

    std::tm *lastPlayed = std::localtime(reinterpret_cast<const time_t *>(&playStats->last_timestamp_user));
    std::strftime(playBuffer, 0x40, strings::get_by_name(strings::names::TITLE_INFO_STRINGS, 3), lastPlayed);
    m_lastPlayed.assign(playBuffer);

    // Calculate play time. We're going to use non-floating point to truncate the remainders.
    int64_t seconds = playStats->playtime / static_cast<int64_t>(1e+9);
    int64_t hours = seconds / 3600;
    int64_t minutes = (seconds % 3600) / 60;
    seconds %= 60;
    m_playTime = stringutil::get_formatted_string(strings::get_by_name(strings::names::TITLE_INFO_STRINGS, 4),
                                                  hours,
                                                  minutes,
                                                  seconds);
}

TitleInfoState::~TitleInfoState()
{
    // Clear this so the next time it's called, the vector is cleared.
    sm_slidePanel->clear_elements();
}

void TitleInfoState::update(void)
{
    // Grab this instead of calling the function over and over.
    bool hasFocus = AppState::has_focus();

    // Update slide panel.
    sm_slidePanel->update(hasFocus);

    // Update the scrolling text.
    m_titleScroll.update(hasFocus);
    m_publisherScroll.update(hasFocus);

    if (input::button_pressed(HidNpadButton_B))
    {
        sm_slidePanel->close();
    }
    else if (sm_slidePanel->is_closed())
    {
        sm_slidePanel->reset();
        AppState::deactivate();
    }
}

void TitleInfoState::render(void)
{
    // This is the vertical gap for rendering text.
    static constexpr int SIZE_VERT_GAP = SIZE_TEXT_TARGET_HEIGHT + 4;
    // This is the size to render the rectangles at.
    static constexpr int SIZE_RECT_WIDTH = SIZE_PANEL_WIDTH - SIZE_PANEL_SUB;

    // This is whether or not the state currently has focus.
    bool hasFocus = AppState::has_focus();

    // Clear the title and publisher targets.
    sm_titleTarget->clear(colors::DIALOG_BOX);
    sm_publisherTarget->clear(colors::CLEAR_COLOR);

    // Grab the panel's target and clear it. To do: This how I originally intended to.
    sm_slidePanel->clear_target();

    // Grab the panel target for rendering to.
    SDL_Texture *panelTarget = sm_slidePanel->get_target();

    // This is our Y rendering coordinate. It's easier to adjust this as needed than change everything.
    // This starts under the icon.
    int y = 280;

    // Start by rendering the icon.
    m_icon->render_stretched(panelTarget, 88, 8, 304, 304);

    // Render the textscroll to the target, then target to the panel target.
    m_titleScroll.render(sm_titleTarget->get(), hasFocus);
    sm_titleTarget->render(panelTarget, 8, (y += SIZE_VERT_GAP));

    // Publisher.
    m_publisherScroll.render(sm_publisherTarget->get(), hasFocus);
    sm_publisherTarget->render(panelTarget, 8, (y += SIZE_VERT_GAP));

    // Application ID.
    sdl::render_rect_fill(panelTarget,
                          8,
                          (y += SIZE_VERT_GAP),
                          SIZE_RECT_WIDTH,
                          SIZE_TEXT_TARGET_HEIGHT,
                          colors::DIALOG_BOX);
    sdl::text::render(panelTarget,
                      16,
                      y + 6,
                      SIZE_FONT,
                      sdl::text::NO_TEXT_WRAP,
                      colors::WHITE,
                      m_applicationID.c_str());

    // Save data ID.
    sdl::render_rect_fill(panelTarget,
                          8,
                          (y += SIZE_VERT_GAP),
                          SIZE_RECT_WIDTH,
                          SIZE_TEXT_TARGET_HEIGHT,
                          colors::CLEAR_COLOR);
    // Text needs to be aligned like the scrolling text.
    sdl::text::render(panelTarget, 16, y + 6, SIZE_FONT, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_saveDataID.c_str());

    // First played.
    sdl::render_rect_fill(panelTarget,
                          8,
                          (y += SIZE_VERT_GAP),
                          SIZE_RECT_WIDTH,
                          SIZE_TEXT_TARGET_HEIGHT,
                          colors::DIALOG_BOX);
    sdl::text::render(panelTarget, 16, y + 6, SIZE_FONT, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_firstPlayed.c_str());

    // Last played.
    sdl::render_rect_fill(panelTarget,
                          8,
                          (y += SIZE_VERT_GAP),
                          SIZE_RECT_WIDTH,
                          SIZE_TEXT_TARGET_HEIGHT,
                          colors::CLEAR_COLOR);
    sdl::text::render(panelTarget, 16, y + 6, SIZE_FONT, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_lastPlayed.c_str());

    // Play time.
    sdl::render_rect_fill(panelTarget,
                          8,
                          (y += SIZE_VERT_GAP),
                          SIZE_RECT_WIDTH,
                          SIZE_TEXT_TARGET_HEIGHT,
                          colors::DIALOG_BOX);
    sdl::text::render(panelTarget, 16, y + 6, SIZE_FONT, sdl::text::NO_TEXT_WRAP, colors::WHITE, m_playTime.c_str());

    // Total launches.
    sdl::render_rect_fill(panelTarget,
                          8,
                          (y += SIZE_VERT_GAP),
                          SIZE_RECT_WIDTH,
                          SIZE_TEXT_TARGET_HEIGHT,
                          colors::CLEAR_COLOR);
    sdl::text::render(panelTarget,
                      16,
                      y + 6,
                      SIZE_FONT,
                      sdl::text::NO_TEXT_WRAP,
                      colors::WHITE,
                      m_totalLaunches.c_str());

    // Save data type.
    sdl::render_rect_fill(panelTarget,
                          8,
                          (y += SIZE_VERT_GAP),
                          SIZE_RECT_WIDTH,
                          SIZE_TEXT_TARGET_HEIGHT,
                          colors::DIALOG_BOX);
    sdl::text::render(panelTarget,
                      16,
                      y + 6,
                      SIZE_FONT,
                      sdl::text::NO_TEXT_WRAP,
                      colors::WHITE,
                      m_saveDataType.c_str());

    sm_slidePanel->render(NULL, AppState::has_focus());
}

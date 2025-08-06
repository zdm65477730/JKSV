#include "appstates/TitleInfoState.hpp"

#include "StateManager.hpp"
#include "colors.hpp"
#include "error.hpp"
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
    : m_user(user)
    , m_titleInfo(titleInfo)
    , m_icon(m_titleInfo->get_icon())
{
    TitleInfoState::initialize_static_members();
    TitleInfoState::create_info_scrolls();
}

TitleInfoState::~TitleInfoState()
{
    // Clear this so the next time it's called, the vector is cleared.
    sm_slidePanel->clear_elements();
}

std::shared_ptr<TitleInfoState> TitleInfoState::create(data::User *user, data::TitleInfo *titleInfo)
{
    return std::make_shared<TitleInfoState>(user, titleInfo);
}

std::shared_ptr<TitleInfoState> TitleInfoState::create_and_push(data::User *user, data::TitleInfo *titleInfo)
{
    auto newState = TitleInfoState::create(user, titleInfo);
    StateManager::push_state(newState);
    return newState;
}

void TitleInfoState::update()
{
    // Grab this instead of calling the function over and over.
    const bool hasFocus = BaseState::has_focus();

    // Update slide panel.
    sm_slidePanel->update(hasFocus);

    if (input::button_pressed(HidNpadButton_B)) { sm_slidePanel->close(); }
    else if (sm_slidePanel->is_closed())
    {
        sm_slidePanel->reset();
        BaseState::deactivate();
    }
}

void TitleInfoState::render()
{
    const bool hasFocus = BaseState::has_focus();
    sm_slidePanel->clear_target();
    sdl::SharedTexture &panelTarget = sm_slidePanel->get_target();

    m_icon->render_stretched(panelTarget, 88, 8, 304, 304);

    sm_slidePanel->render(sdl::Texture::Null, hasFocus);
}

void TitleInfoState::initialize_static_members()
{
    if (!sm_slidePanel)
    {
        sm_slidePanel = std::make_unique<ui::SlideOutPanel>(SIZE_PANEL_WIDTH, ui::SlideOutPanel::Side::Right);
    }
}

void TitleInfoState::create_info_scrolls()
{
    static constexpr int SIZE_VERT_GAP = SIZE_TEXT_TARGET_HEIGHT + 4;
    static constexpr int COORD_INIT_Y  = 278;

    const uint64_t applicationID       = m_titleInfo->get_application_id();
    const FsSaveDataInfo *saveInfo     = m_user->get_save_info_by_id(applicationID);
    const PdmPlayStatistics *playStats = m_user->get_play_stats_by_id(applicationID);
    if (error::is_null(saveInfo) || error::is_null(playStats)) { return; }

    const char *appIDFormat       = strings::get_by_name(strings::names::TITLEINFO, 0);
    const char *saveIDFormat      = strings::get_by_name(strings::names::TITLEINFO, 1);
    const char *firstPlayedFormat = strings::get_by_name(strings::names::TITLEINFO, 2);
    const char *lastPlayedFormat  = strings::get_by_name(strings::names::TITLEINFO, 3);
    const char *playTimeFormat    = strings::get_by_name(strings::names::TITLEINFO, 4);
    const char *launchesFormat    = strings::get_by_name(strings::names::TITLEINFO, 5);
    const char *saveTypeFormat    = strings::get_by_name(strings::names::TITLEINFO, 6);

    // Gonna push this all here and loop it later to make this "easier."
    std::vector<std::string_view> textVector{};
    textVector.push_back(m_titleInfo->get_title());
    textVector.push_back(m_titleInfo->get_publisher());

    const std::string appIDString = stringutil::get_formatted_string(appIDFormat, applicationID);
    textVector.push_back(appIDString);

    const std::string saveIDString = stringutil::get_formatted_string(saveIDFormat, saveInfo->save_data_id);
    textVector.push_back(saveIDString);

    std::array<char, 0x40> firstPlayed = {0};
    const std::time_t firstTime        = static_cast<const std::time_t>(playStats->first_timestamp_user);
    const std::tm *firstLocal          = std::localtime(&firstTime);
    std::strftime(firstPlayed.data(), 0x40, firstPlayedFormat, firstLocal);
    textVector.push_back(firstPlayed.data());

    std::array<char, 0x40> lastPlayed = {0};
    const std::time_t lastTime        = static_cast<const std::time_t>(playStats->last_timestamp_user);
    const std::tm *lastLocal          = std::localtime(&lastTime);
    std::strftime(lastPlayed.data(), 0x40, lastPlayedFormat, lastLocal);
    textVector.push_back(lastPlayed.data());

    int64_t seconds = playStats->playtime / 1000000000;
    int64_t hours   = seconds / 3600;
    int64_t minutes = (seconds % 3600) / 60;
    seconds %= 60;
    const std::string playTimeString = stringutil::get_formatted_string(playTimeFormat, hours, minutes, seconds);
    textVector.push_back(playTimeString);

    const std::string launchesString = stringutil::get_formatted_string(launchesFormat, playStats->total_launches);
    textVector.push_back(launchesString);

    const char *saveType             = strings::get_by_name(strings::names::SAVE_DATA_TYPES, saveInfo->save_data_type);
    const std::string saveTypeString = stringutil::get_formatted_string(saveTypeFormat, saveType);
    textVector.push_back(saveTypeString);

    // Just loop, I guess.
    int y = COORD_INIT_Y;
    for (const std::string_view &string : textVector)
    {
        auto newField = TitleInfoState::create_new_scroll(string, (y += SIZE_VERT_GAP));
        sm_slidePanel->push_new_element(newField);
    }
}

std::shared_ptr<ui::TextScroll> TitleInfoState::create_new_scroll(std::string_view text, int y)
{
    static constexpr int SIZE_FIELD_WIDTH = SIZE_PANEL_WIDTH - SIZE_PANEL_SUB;
    auto textFieldScroll                  = std::make_shared<ui::TextScroll>(text,
                                                            8,
                                                            y,
                                                            SIZE_FIELD_WIDTH,
                                                            SIZE_TEXT_TARGET_HEIGHT,
                                                            SIZE_FONT,
                                                            colors::WHITE,
                                                            m_fieldClear ? colors::CLEAR_COLOR : colors::DIALOG_BOX,
                                                            false);
    m_fieldClear                          = m_fieldClear ? false : true;
    return textFieldScroll;
}

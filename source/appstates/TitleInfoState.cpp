#include "appstates/TitleInfoState.hpp"

#include "StateManager.hpp"
#include "error.hpp"
#include "fs/save_data_functions.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"

#include <ctime>

namespace
{
    /// @brief This is the width of the panel in pixels.
    constexpr int SIZE_FIELD_WIDTH = 456;
    /// @brief This is the vertical size in pixels of the render targets for title/publisher.
    constexpr int SIZE_FIELD_HEIGHT = 44;
    /// @brief This is the vertical gap in between fields.
    static constexpr int VERT_GAP = SIZE_FIELD_HEIGHT + 12;

    /// @brief This is the gap on both sides
    constexpr int SIZE_PANEL_SUB = 16;

    /// @brief This is the size used for rendering text here.
    constexpr int SIZE_FONT = 24;

    /// @brief Dimensions of the frame.
    constexpr int SIZE_START_WIDTH_HEIGHT = 16;
    constexpr int SIZE_END_WIDTH          = 960;
    constexpr int SIZE_END_HEIGHT         = 580;

    /// @brief Buffer size used for std::strftime.
    constexpr size_t SIZE_C_BUFFER = 0x40;
} // namespace

// Defined at bottom
static inline std::shared_ptr<ui::TextScroll> create_new_field(std::string_view text, int x, int y, sdl::Color clear);
static inline std::shared_ptr<ui::TextScroll> create_new_field(std::string &text, int x, int y, sdl::Color clear);
static bool is_a_best_game(uint64_t applicationID) noexcept;

TitleInfoState::TitleInfoState(data::User *user, data::TitleInfo *titleInfo, const FsSaveDataInfo *saveInfo)
    : m_user(user)
    , m_titleInfo(titleInfo)
    , m_saveInfo(saveInfo)
    , m_icon(m_titleInfo->get_icon())
    , m_transition(0,
                   0,
                   SIZE_START_WIDTH_HEIGHT,
                   SIZE_START_WIDTH_HEIGHT,
                   0,
                   0,
                   SIZE_END_WIDTH,
                   SIZE_END_HEIGHT,
                   m_transition.DEFAULT_THRESHOLD)
{
    TitleInfoState::initialize_static_members();
    TitleInfoState::initialize_info_fields();
}

void TitleInfoState::update()
{
    // Grab this instead of calling the function over and over.
    const bool hasFocus = BaseState::has_focus();
    m_transition.update();
    sm_frame->set_from_transition(m_transition, true);
    if (!m_transition.in_place()) { return; }
    else if (m_transition.in_place() && !m_timerStarted)
    {
        m_timer.start(40);
        m_timerStarted = true;
    }

    TitleInfoState::update_field_count();
    TitleInfoState::update_fields(hasFocus);

    bool bPressed = input::button_pressed(HidNpadButton_B);
    if (bPressed) { TitleInfoState::close(); }
    else if (m_close && m_transition.in_place()) { TitleInfoState::deactivate_state(); }
}

void TitleInfoState::render()
{
    const bool hasFocus = BaseState::has_focus();
    sm_frame->render(sdl::Texture::Null, hasFocus);
    if (!m_transition.in_place()) { return; }

    for (int i = 0; i < m_fieldDisplayCount; i++)
    {
        auto &currentField = m_infoFields[i];
        currentField->render(sdl::Texture::Null, hasFocus);
    }
}

void TitleInfoState::initialize_static_members()
{
    if (sm_frame)
    {
        sm_frame->set_from_transition(m_transition, true);
        return;
    }

    const int x      = m_transition.get_centered_x();
    const int y      = m_transition.get_centered_y();
    const int width  = m_transition.get_width();
    const int height = m_transition.get_height();
    sm_frame         = ui::Frame::create(x, y, width, height);
}

void TitleInfoState::initialize_info_fields()
{
    // This is the gap used for vertical spacing.
    static constexpr int X_A     = 176; // X for left column.
    static constexpr int X_B     = 648; // X for right column.
    static constexpr int Y_START = 88;

    const uint64_t applicationID       = m_titleInfo->get_application_id();
    const PdmPlayStatistics *playStats = m_user->get_play_stats_by_id(applicationID);

    FsSaveDataExtraData extraData{};
    fs::read_save_extra_data(m_saveInfo, extraData);

    // This is easier and simpler than manually adjusting this all of the time.
    int y = Y_START;
    TitleInfoState::create_title(y);
    y += VERT_GAP;

    // I wanted them to alternate color be column.
    TitleInfoState::create_application_id(extraData, X_A, y);
    TitleInfoState::create_owner_id(extraData, X_A, y);
    TitleInfoState::create_journal(extraData, X_A, y);
    TitleInfoState::create_timestamp(extraData, X_A, y);
    int secondY = y + VERT_GAP; // This is used to line up the bottom set of info.

    y = Y_START + VERT_GAP;
    TitleInfoState::create_save_data_id(X_B, y);
    TitleInfoState::create_size(extraData, X_B, y);
    TitleInfoState::create_commit_id(extraData, X_B, y);

    y = secondY;
    TitleInfoState::create_user(extraData, X_A, y);
    TitleInfoState::create_first_played(playStats, X_A, y);
    TitleInfoState::create_launched(playStats, X_A, y);
    TitleInfoState::create_save_data_type(m_saveInfo, X_A, y);

    y = secondY + VERT_GAP;
    TitleInfoState::create_last_played(playStats, X_B, y);
    TitleInfoState::create_play_time(playStats, X_B, y);
}

void TitleInfoState::create_title(int y)
{
    static constexpr std::string BESTEST_STAR = "$\u2605$";
    static constexpr int WIDTH                = SIZE_END_WIDTH - 64;
    static constexpr int X                    = 640 - (WIDTH / 2);
    static constexpr int TITLE_FIELD_HEIGHT   = 44;
    static constexpr int TITLE_FONT_SIZE      = 26;

    std::shared_ptr<ui::TextScroll> field{};
    const char *title            = m_titleInfo->get_title();
    const uint64_t applicationID = m_titleInfo->get_application_id();
    if (is_a_best_game(applicationID))
    {
        std::string bestTitle = BESTEST_STAR + " " + title + " " + BESTEST_STAR;
        field                 = ui::TextScroll::create(bestTitle,
                                       X,
                                       y,
                                       WIDTH,
                                       TITLE_FIELD_HEIGHT,
                                       TITLE_FONT_SIZE,
                                       colors::WHITE,
                                       colors::TRANSPARENT);
    }
    else
    {
        field =
            ui::TextScroll::create(title, X, y, WIDTH, TITLE_FIELD_HEIGHT, TITLE_FONT_SIZE, colors::WHITE, colors::TRANSPARENT);
    }

    m_infoFields.push_back(field);
}

void TitleInfoState::create_application_id(const FsSaveDataExtraData &extraData, int x, int &y)
{
    const char *applicationIDFormat = strings::get_by_name(strings::names::TITLEINFO, 0);
    std::string applicationID       = stringutil::get_formatted_string(applicationIDFormat, extraData.attr.application_id);

    auto field = create_new_field(applicationID, x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_save_data_id(int x, int &y)
{
    const char *saveIDFormat = strings::get_by_name(strings::names::TITLEINFO, 1);
    std::string saveID       = stringutil::get_formatted_string(saveIDFormat, m_saveInfo->save_data_id);

    auto field = create_new_field(saveID, x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_owner_id(const FsSaveDataExtraData &extraData, int x, int &y)
{
    const char *ownerIDFormat = strings::get_by_name(strings::names::TITLEINFO, 2);
    std::string ownerString   = stringutil::get_formatted_string(ownerIDFormat, extraData.owner_id);

    auto field = create_new_field(ownerString, x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_size(const FsSaveDataExtraData &extraData, int x, int &y)
{
    const char *sizeFormat = strings::get_by_name(strings::names::TITLEINFO, 3);
    std::string sizeString = stringutil::get_formatted_string(sizeFormat, extraData.data_size);

    auto field = create_new_field(sizeString, x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_journal(const FsSaveDataExtraData &extraData, int x, int &y)
{
    const char *journalFormat = strings::get_by_name(strings::names::TITLEINFO, 4);
    std::string journalString = stringutil::get_formatted_string(journalFormat, extraData.journal_size);

    auto field = create_new_field(journalString, x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_commit_id(const FsSaveDataExtraData &extraData, int x, int &y)
{
    const char *commitIDFormat = strings::get_by_name(strings::names::TITLEINFO, 5);
    std::string commitIDString = stringutil::get_formatted_string(commitIDFormat, extraData.commit_id);

    auto field = create_new_field(commitIDString, x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_timestamp(const FsSaveDataExtraData &extraData, int x, int &y)
{
    const char *timestampFormat = strings::get_by_name(strings::names::TITLEINFO, 6);

    const std::time_t stampTime = static_cast<std::time_t>(extraData.timestamp);
    std::tm stampTm             = *std::localtime(&stampTime);

    std::array<char, SIZE_C_BUFFER> buffer = {0};
    std::strftime(buffer.data(), SIZE_C_BUFFER, timestampFormat, &stampTm);

    auto field = create_new_field(buffer.data(), x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_user(const FsSaveDataExtraData &extraData, int x, int &y)
{
    const uint64_t uidA    = extraData.attr.uid.uid[0];
    const uint64_t uidB    = extraData.attr.uid.uid[1];
    const char *userFormat = strings::get_by_name(strings::names::TITLEINFO, 7);
    std::string userString = stringutil::get_formatted_string(userFormat, uidA, uidB);

    // This one differs from the rest... so
    auto field = ui::TextScroll::create(userString,
                                        x,
                                        y,
                                        SIZE_FIELD_WIDTH * 2 + 16, // Need to account for the gap.
                                        SIZE_FIELD_HEIGHT,
                                        SIZE_FONT,
                                        colors::WHITE,
                                        TitleInfoState::get_field_color(),
                                        true);
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_first_played(const PdmPlayStatistics *playStats, int x, int &y)
{
    const char *firstPlayedFormat = strings::get_by_name(strings::names::TITLEINFO, 8);

    const time_t firstPlayed = static_cast<std::time_t>(playStats->first_timestamp_user);
    std::tm firstPlayedTm    = *std::localtime(&firstPlayed);

    std::array<char, SIZE_C_BUFFER> buffer = {0};
    std::strftime(buffer.data(), SIZE_C_BUFFER, firstPlayedFormat, &firstPlayedTm);

    auto field = create_new_field(buffer.data(), x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_last_played(const PdmPlayStatistics *playStats, int x, int &y)
{
    const char *lastPlayedFormat = strings::get_by_name(strings::names::TITLEINFO, 9);

    const std::time_t lastPlayed = static_cast<std::time_t>(playStats->last_timestamp_user);
    std::tm lastPlayedTm         = *std::localtime(&lastPlayed);

    std::array<char, SIZE_C_BUFFER> buffer = {0};
    std::strftime(buffer.data(), SIZE_C_BUFFER, lastPlayedFormat, &lastPlayedTm);

    auto field = create_new_field(buffer.data(), x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_play_time(const PdmPlayStatistics *playStats, int x, int &y)
{
    const char *playTitleFormat = strings::get_by_name(strings::names::TITLEINFO, 10);

    int64_t seconds       = playStats->playtime / 1000000000;
    const int64_t hours   = seconds / 3600;
    const int64_t minutes = (seconds % 3600) / 60;
    seconds %= 60;

    std::string playTimeString = stringutil::get_formatted_string(playTitleFormat, hours, minutes, seconds);

    auto field = create_new_field(playTimeString, x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_launched(const PdmPlayStatistics *playStats, int x, int &y)
{
    const char *launchedFormat = strings::get_by_name(strings::names::TITLEINFO, 11);
    std::string launchesString = stringutil::get_formatted_string(launchedFormat, playStats->total_launches);

    auto field = create_new_field(launchesString, x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::create_save_data_type(const FsSaveDataInfo *saveInfo, int x, int &y)
{
    const char *saveTypeFormat = strings::get_by_name(strings::names::TITLEINFO, 12);
    const char *saveType       = strings::get_by_name(strings::names::SAVE_DATA_TYPES, saveInfo->save_data_type);

    std::string saveTypeString = stringutil::get_formatted_string(saveTypeFormat, saveType);

    auto field = create_new_field(saveTypeString, x, y, TitleInfoState::get_field_color());
    m_infoFields.push_back(field);

    y += VERT_GAP;
}

void TitleInfoState::update_field_count() noexcept
{
    const int fieldSize = m_infoFields.size();
    if (m_fieldDisplayCount >= fieldSize || !m_timer.is_triggered()) { return; }

    ++m_fieldDisplayCount;
}

void TitleInfoState::update_fields(bool hasFocus) noexcept
{
    for (int i = 0; i < m_fieldDisplayCount; i++)
    {
        auto &field = m_infoFields[i];
        field->update(hasFocus);
    }
}

sdl::Color TitleInfoState::get_field_color() noexcept
{
    m_fieldClearSwitch = m_fieldClearSwitch ? false : true;
    return m_fieldClearSwitch ? colors::DIALOG_DARK : colors::CLEAR_COLOR;
}

void TitleInfoState::close() noexcept
{
    m_transition.set_target_width(SIZE_START_WIDTH_HEIGHT);
    m_transition.set_target_height(SIZE_START_WIDTH_HEIGHT);
    m_close = true;
}

void TitleInfoState::deactivate_state() { BaseState::deactivate(); }

static inline std::shared_ptr<ui::TextScroll> create_new_field(std::string_view text, int x, int y, sdl::Color clear)
{
    return ui::TextScroll::create(text, x, y, SIZE_FIELD_WIDTH, SIZE_FIELD_HEIGHT, SIZE_FONT, colors::WHITE, clear, false);
}

static inline std::shared_ptr<ui::TextScroll> create_new_field(std::string &text, int x, int y, sdl::Color clear)
{
    return ui::TextScroll::create(text, x, y, SIZE_FIELD_WIDTH, SIZE_FIELD_HEIGHT, SIZE_FONT, colors::WHITE, clear, false);
}

static bool is_a_best_game(uint64_t applicationID) noexcept
{
    static constexpr std::array<uint64_t, 16> BESTEST_GAMES = {0x0100BC300CB48000,
                                                               0x01006C300E9F0000,
                                                               0x010065301A2E0000,
                                                               0x01000EA014150000,
                                                               0x01004B301415A000,
                                                               0x0100AA001415E000,
                                                               0x0100A5B00BDC6000,
                                                               0x01007EF00B094000,
                                                               0x01005C60086BE000,
                                                               0x0100770008DD8000,
                                                               0x0100B04011742000,
                                                               0x0100BC0018138000,
                                                               0x01002C0008E52000,
                                                               0x0100FF500E34A000,
                                                               0x010015100B514000,
                                                               0x0100AC20128AC000};

    return std::find(BESTEST_GAMES.begin(), BESTEST_GAMES.end(), applicationID) != BESTEST_GAMES.end();
}
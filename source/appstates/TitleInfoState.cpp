#include "appstates/TitleInfoState.hpp"
#include "colors.hpp"
#include "input.hpp"
#include "sdl.hpp"

namespace
{
    /// @brief This is the width of the panel in pixels.
    constexpr int SIZE_PANEL_WIDTH = 480;
} // namespace

TitleInfoState::TitleInfoState(data::User *user, data::TitleInfo *titleInfo)
    : m_user(user), m_titleInfo(titleInfo), m_icon(m_titleInfo->get_icon())
{
    // This needs to be checked. All title information panels share these members.
    if (!sm_initialized)
    {
        // This is the slide panel everything is rendered to.
        sm_slidePanel = std::make_unique<ui::SlideOutPanel>(SIZE_PANEL_WIDTH, ui::SlideOutPanel::Side::Right);
        sm_titleTarget = sdl::TextureManager::create_load_texture("infoTitleTarget",
                                                                  SIZE_PANEL_WIDTH - 32,
                                                                  32,
                                                                  SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
        sm_initialized = true;
    }
}

TitleInfoState::~TitleInfoState()
{
    // Clear this so the next time it's called, the vector is cleared.
    sm_slidePanel->clear_elements();
}

void TitleInfoState::update(void)
{
    // Update slide panel.
    sm_slidePanel->update(AppState::has_focus());

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
    // Grab the panel's target and clear it. To do: This how I originally intended to.
    sm_slidePanel->clear_target();

    // Grab the panel target for rendering to.
    SDL_Texture *panelTarget = sm_slidePanel->get_target();

    // Start by rendering the icon.
    m_icon->render(panelTarget, 112, 24);

    // Clear the title target and render the text scroll to it.
    sm_titleTarget->clear(colors::DIALOG_BOX);


    sm_slidePanel->render(NULL, AppState::has_focus());
}

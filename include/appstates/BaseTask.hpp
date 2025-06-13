#pragma once
#include "appstates/AppState.hpp"
#include "system/Timer.hpp"
#include "ui/ColorMod.hpp"
#include <array>
#include <string_view>

/// @brief Normally, I wouldn't do this, but this holds a single function both TaskState and ProgressState share...
class BaseTask : public AppState
{
    public:
        /// @brief Constructor. Starts the glyph timer and sets AppState to not allow closing.
        BaseTask(void);

        /// @brief Virtual destructor.
        virtual ~BaseTask() {};

        /// @brief Runs the update routine for rendering the loading glyph animation.
        /// @param
        void update(void) override;

        /// @brief Virtual render function.
        virtual void render(void) = 0;

        /// @brief This function renders the loading glyph in the bottom left corner.
        /// @note This is mostly just so users don't think JKSV has frozen when operations take a long time.
        void render_loading_glyph(void);

    private:
        /// @brief This is the current frame of the loading glyph animation.
        int m_currentFrame = 0;

        /// @brief This is the timer used for changing the current glyph/frame of the animation.
        sys::Timer m_frameTimer;

        /// @brief This is used to give the animation its pulsing color.
        ui::ColorMod m_colorMod;

        /// @brief This array holds the glyphs of the loading sequence. I think it's from the Wii?
        static inline std::array<std::string_view, 8> sm_glyphArray =
            {"\ue020", "\ue021", "\ue022", "\ue023", "\ue024", "\ue025", "\ue026", "\ue027"};
};

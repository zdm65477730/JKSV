#pragma once
#include "sdl.hpp"
#include "ui/Element.hpp"
#include "ui/Transition.hpp"

namespace ui
{
    class ControlGuide final : public ui::Element
    {
        public:
            /// @brief Creates a new control guide.
            /// @param string Pointer to the control guide string to render.
            ControlGuide(const char *guide);

            static inline std::shared_ptr<ControlGuide> create(const char *guide)
            {
                return std::make_shared<ControlGuide>(guide);
            }

            /// @brief Update routine. Opens the guide.
            void update(bool hasFocus) override;

            /// @brief Sub update routine. Handles hiding the control guide.
            void sub_update();

            /// @brief Renders the control guide. Both arguments are ignored in this case.
            void render(sdl::SharedTexture &target, bool hasFocus) override;

        private:
            /// @brief Stores the pointer to the guide string.
            const char *m_guide{};

            /// @brief Width of the control guide text.
            int m_textWidth{};

            /// @brief This stores the target X coordinate of the guide.
            int m_targetX{};

            /// @brief Width of the guide.
            int m_guideWidth{};

            /// @brief Transition for the guide string.
            ui::Transition m_transition{};

            /// @brief This is shared by all instances.
            static inline sdl::SharedTexture sm_controlCap{};

            /// @brief Ensures the control guide cap is loaded for all instances.
            void initialize_static_members();
    };
}
#pragma once
#include "SDL.hpp"
#include "UI/Element.hpp"
#include <vector>

namespace UI
{
    class SlideOutPanel : public UI::Element
    {
        public:
            // Which side the panel the slides out from.
            enum class Side
            {
                Left,
                Right
            };
            // Width of panel, which side it "spawns" from.
            SlideOutPanel(int Width, SlideOutPanel::Side Side);
            ~SlideOutPanel() {};

            void Update(bool HasFocus);
            void Render(SDL_Texture *Target, bool HasFocus);
            // Clears the panel target to a semi-transparent black.
            void ClearTarget(void);
            // Resets panel back to default values.
            void Reset(void);
            // Closes panel.
            void Close(void);
            // Returns whether or not panel is fully open.
            bool IsOpen(void) const;
            // Returns whether or not the panel is fully closed.
            bool IsClosed(void) const;
            // Adds new element to vector.
            void PushNewElement(std::shared_ptr<UI::Element> NewElement);
            // Returns pointer to render target to allow rendering besides elements pushed.
            SDL_Texture *Get(void);

        private:
            // X coordinate
            double m_X;
            // Width of panel.
            int m_Width;
            // Side the panel spawned from.
            SlideOutPanel::Side m_Side;
            // Vector of elements.
            std::vector<std::shared_ptr<UI::Element>> m_Elements;
            // Bool for whether panel is open or not.
            bool m_IsOpen = false;
            // Whether or not to close panel.
            bool m_ClosePanel = false;
            // Render target
            SDL::SharedTexture m_RenderTarget;
    };
} // namespace UI

#include "ui/ColorMod.hpp"

void ui::ColorMod::update(void)
{
    if (m_direction && (m_colorMod += 6) >= 0x72)
    {
        m_direction = false;
    }
    else if (!m_direction && (m_colorMod -= 3) <= 0x00)
    {
        m_direction = true;
    }
}

ui::ColorMod::operator sdl::Color(void) const
{
    return {static_cast<uint32_t>((0x88 + m_colorMod) << 16 | (0xC5 + (m_colorMod / 2)) << 8 | 0xFF)};
}

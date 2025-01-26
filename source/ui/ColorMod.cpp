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

ui::ColorMod::operator uint8_t(void) const
{
    return m_colorMod;
}

#include "ui/ColorMod.hpp"

//                      ---- Public functions ----

void ui::ColorMod::update() noexcept
{
    const bool changeDown = m_direction && ((m_colorMod += 6) >= 0x72);
    const bool changeUp   = !m_direction && ((m_colorMod -= 3) <= 0x00);
    if (changeDown) { m_direction = false; }
    else if (changeUp) { m_direction = true; }
}

ui::ColorMod::operator sdl::Color() const noexcept
{
    uint32_t color{};
    color |= static_cast<uint32_t>((0x88 + m_colorMod) << 16);
    color |= static_cast<uint64_t>((0xC5 + (m_colorMod / 2)) << 8);
    color |= 0xFF;
    return sdl::Color{color};
}

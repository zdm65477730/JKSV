#include "ui/BoundingBox.hpp"

namespace
{
    constexpr int CORNER_WIDTH  = 8;
    constexpr int CORNER_HEIGHT = 8;

    constexpr int RECT_WIDTH  = 4;
    constexpr int RECT_HEIGHT = 4;
}

ui::BoundingBox::BoundingBox(int x, int y, int width, int height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
{
    BoundingBox::initialize_static_members();
}

std::shared_ptr<ui::BoundingBox> ui::BoundingBox::create(int x, int y, int width, int height)
{
    return std::make_shared<ui::BoundingBox>(x, y, width, height);
}

void ui::BoundingBox::update(bool hasFocus) { m_colorMod.update(); }

void ui::BoundingBox::render(sdl::SharedTexture &target, bool hasFocus)
{
    sm_corners->set_color_mod(m_colorMod);

    const int rightX     = (m_x + m_width) - CORNER_WIDTH;
    const int rightRectX = (m_x + m_width) - RECT_WIDTH;

    const int midX        = m_x + CORNER_WIDTH;
    const int midY        = m_y + CORNER_HEIGHT;
    const int midWidth    = m_width - (CORNER_WIDTH * 2);
    const int midHeight   = m_height - (CORNER_HEIGHT * 2);
    const int bottomY     = (m_y + m_height) - CORNER_HEIGHT;
    const int bottomRectY = (m_y + m_height) - RECT_HEIGHT;

    sm_corners->render_part(target, m_x, m_y, 0, 0, CORNER_WIDTH, CORNER_HEIGHT);
    sdl::render_rect_fill(target, midX, m_y, midWidth, RECT_HEIGHT, m_colorMod);
    sm_corners->render_part(target, rightX, m_y, CORNER_HEIGHT, 0, CORNER_WIDTH, CORNER_HEIGHT);
    // Middle
    sdl::render_rect_fill(target, m_x, midY, RECT_WIDTH, midHeight, m_colorMod);
    sdl::render_rect_fill(target, rightRectX, midY, RECT_WIDTH, midHeight, m_colorMod);
    // Bottom
    sm_corners->render_part(target, m_x, bottomY, 0, CORNER_HEIGHT, CORNER_WIDTH, CORNER_HEIGHT);
    sdl::render_rect_fill(target, midX, bottomRectY, midWidth, RECT_HEIGHT, m_colorMod);
    sm_corners->render_part(target, rightX, bottomY, CORNER_WIDTH, CORNER_HEIGHT, CORNER_WIDTH, CORNER_HEIGHT);
}

void ui::BoundingBox::set_xy(int x, int y)
{
    if (x != BoundingBox::NO_SET) { m_x = x; }
    if (y != BoundingBox::NO_SET) { m_y = y; }
}

void ui::BoundingBox::set_width_height(int width, int height)
{
    if (width != BoundingBox::NO_SET) { m_width = width; }
    if (height != BoundingBox::NO_SET) { m_height = height; }
}

void ui::BoundingBox::initialize_static_members()
{
    if (sm_corners) { return; }
    sm_corners = sdl::TextureManager::create_load_texture("menuCorners", "romfs:/Textures/MenuBounding.png");
}

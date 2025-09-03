#include "ui/DialogBox.hpp"

#include "graphics/colors.hpp"

namespace
{
    constexpr int CORNER_WIDTH  = 16;
    constexpr int CORNER_HEIGHT = 16;
}

ui::DialogBox::DialogBox(int x, int y, int width, int height, ui::DialogBox::Type type)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_type(type)
{
    ui::DialogBox::initialize_static_members();
}

void ui::DialogBox::render(sdl::SharedTexture &target, bool hasFocus)
{
    const bool darkDialog       = m_type == DialogBox::Type::Dark;
    sdl::SharedTexture &corners = darkDialog ? sm_darkCorners : sm_lightCorners;
    const sdl::Color rectColor  = darkDialog ? colors::DIALOG_DARK : colors::DIALOG_LIGHT;

    // Top
    corners->render_part(target, m_x, m_y, 0, 0, CORNER_WIDTH, CORNER_HEIGHT);
    sdl::render_rect_fill(target, m_x + CORNER_WIDTH, m_y, m_width - (CORNER_WIDTH * 2), CORNER_HEIGHT, rectColor);
    corners->render_part(target, (m_x + m_width) - CORNER_WIDTH, m_y, CORNER_WIDTH, 0, CORNER_WIDTH, CORNER_HEIGHT);

    // Middle
    sdl::render_rect_fill(target, m_x, m_y + CORNER_HEIGHT, m_width, m_height - (CORNER_HEIGHT * 2), rectColor);

    // Bottom
    corners->render_part(target, m_x, (m_y + m_height) - CORNER_HEIGHT, 0, CORNER_HEIGHT, CORNER_WIDTH, CORNER_HEIGHT);
    sdl::render_rect_fill(target,
                          m_x + CORNER_WIDTH,
                          (m_y + m_height) - CORNER_HEIGHT,
                          m_width - (CORNER_WIDTH * 2),
                          CORNER_HEIGHT,
                          rectColor);
    corners->render_part(target,
                         (m_x + m_width) - CORNER_WIDTH,
                         (m_y + m_height) - CORNER_HEIGHT,
                         CORNER_WIDTH,
                         CORNER_HEIGHT,
                         CORNER_WIDTH,
                         CORNER_HEIGHT);
}

void ui::DialogBox::set_x(int x) { m_x = x; }

void ui::DialogBox::set_y(int y) { m_y = y; }

void ui::DialogBox::set_width(int width) { m_width = width; }

void ui::DialogBox::set_height(int height) { m_height = height; }

void ui::DialogBox::initialize_static_members()
{
    if (sm_darkCorners && sm_lightCorners) { return; }

    sm_darkCorners  = sdl::TextureManager::load("darkCorners", "romfs:/Textures/DialogCornersDark.png");
    sm_lightCorners = sdl::TextureManager::load("lightCorners", "romfs:/Textures/DialogCornersLight.png");
}

#include "ui/DialogBox.hpp"

#include "colors.hpp"

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

std::shared_ptr<ui::DialogBox> ui::DialogBox::create(int x, int y, int width, int height, ui::DialogBox::Type type)
{
    return std::make_shared<ui::DialogBox>(x, y, width, height, type);
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

void ui::DialogBox::set_xy(int x, int y)
{
    if (x != DialogBox::NO_SET) { m_x = x; }
    if (y != DialogBox::NO_SET) { m_y = y; }
}

void ui::DialogBox::set_width_height(int width, int height)
{
    if (width != DialogBox::NO_SET) { m_width = width; }
    if (height != DialogBox::NO_SET) { m_height = height; }
}

void ui::DialogBox::initialize_static_members()
{
    if (sm_darkCorners && sm_lightCorners) { return; }

    sm_darkCorners  = sdl::TextureManager::create_load_texture("darkCorners", "romfs:/Textures/DialogCornersDark.png");
    sm_lightCorners = sdl::TextureManager::create_load_texture("lightCorners", "romfs:/Textures/DialogCornersLight.png");
}

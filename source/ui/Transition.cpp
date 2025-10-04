#include "ui/Transition.hpp"

#include "config/config.hpp"
#include "logging/logger.hpp"
#include "mathutil.hpp"

#include <cmath>

ui::Transition::Transition()
    : m_scaling(config::get_animation_scaling()) {};

ui::Transition::Transition(int x,
                           int y,
                           int width,
                           int height,
                           int targetX,
                           int targetY,
                           int targetWidth,
                           int targetHeight,
                           int threshold) noexcept
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_targetX(targetX)
    , m_targetY(targetY)
    , m_targetWidth(targetWidth)
    , m_targetHeight(targetHeight)
    , m_threshold(threshold)
    , m_scaling(config::get_animation_scaling()) {};

void ui::Transition::update() noexcept
{
    ui::Transition::update_x_coord();
    ui::Transition::update_y_coord();
    ui::Transition::update_width();
    ui::Transition::update_height();
}

void ui::Transition::update_xy() noexcept
{
    ui::Transition::update_x_coord();
    ui::Transition::update_y_coord();
}

void ui::Transition::update_width_height() noexcept
{
    ui::Transition::update_width();
    ui::Transition::update_height();
}

bool ui::Transition::in_place() const noexcept
{
    return m_x == m_targetX && m_y == m_targetY && m_width == m_targetWidth && m_height == m_targetHeight;
}

bool ui::Transition::in_place_xy() const noexcept { return m_x == m_targetX && m_y == m_targetY; }

bool ui::Transition::in_place_width_height() const noexcept { return m_width == m_targetWidth && m_height == m_targetHeight; }

int ui::Transition::get_x() const noexcept { return static_cast<int>(m_x); }

int ui::Transition::get_y() const noexcept { return static_cast<int>(m_y); }

int ui::Transition::get_width() const noexcept { return static_cast<int>(m_width); }

int ui::Transition::get_height() const noexcept { return static_cast<int>(m_height); }

int ui::Transition::get_target_x() const noexcept { return static_cast<int>(m_targetX); }

int ui::Transition::get_target_y() const noexcept { return static_cast<int>(m_targetY); }

int ui::Transition::get_target_width() const noexcept { return static_cast<int>(m_targetWidth); }

int ui::Transition::get_target_height() const noexcept { return static_cast<int>(m_targetHeight); }

int ui::Transition::get_centered_x() const noexcept { return 640 - (static_cast<int>(m_width) / 2); }

int ui::Transition::get_centered_y() const noexcept { return 360 - (static_cast<int>(m_height) / 2); }

void ui::Transition::set_x(int x) noexcept { m_x = x; }

void ui::Transition::set_y(int y) noexcept { m_y = y; }

void ui::Transition::set_width(int width) noexcept { m_width = static_cast<double>(width); }

void ui::Transition::set_height(int height) noexcept { m_height = static_cast<double>(height); }

void ui::Transition::set_target_x(int targetX) noexcept { m_targetX = static_cast<double>(targetX); }

void ui::Transition::set_target_y(int targetY) noexcept { m_targetY = static_cast<double>(targetY); }

void ui::Transition::set_target_width(int targetWidth) noexcept { m_targetWidth = static_cast<double>(targetWidth); }

void ui::Transition::set_target_height(int targetHeight) noexcept { m_targetHeight = static_cast<double>(targetHeight); }

void ui::Transition::set_threshold(int threshold) noexcept { m_threshold = static_cast<double>(threshold); }

void ui::Transition::update_x_coord() noexcept
{
    if (m_x == m_targetX) { return; }

    const double add = (m_targetX - m_x) / m_scaling;
    m_x += std::round(add);

    const double distance = math::Util<double>::absolute_distance(m_x, m_targetX);
    if (distance <= m_threshold) { m_x = m_targetX; }
}

void ui::Transition::update_y_coord() noexcept
{
    if (m_y == m_targetY) { return; }

    const double add = (m_targetY - m_y) / m_scaling;
    m_y += std::round(add);

    const double distance = math::Util<double>::absolute_distance(m_y, m_targetY);
    if (distance <= m_threshold) { m_y = m_targetY; }
}

void ui::Transition::update_width() noexcept
{
    if (m_width == m_targetWidth) { return; }

    const double add = (m_targetWidth - m_width) / m_scaling;
    m_width += std::round(add);

    const double distance = math::Util<double>::absolute_distance(m_width, m_targetWidth);
    if (distance <= m_threshold) { m_width = m_targetWidth; }
}

void ui::Transition::update_height() noexcept
{
    if (m_height == m_targetHeight) { return; }

    const double add = (m_targetHeight - m_height) / m_scaling;
    m_height += add;

    const double distance = math::Util<double>::absolute_distance(m_height, m_targetHeight);
    if (distance <= m_threshold) { m_height = m_targetHeight; }
}

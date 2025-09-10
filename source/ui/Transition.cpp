#include "ui/Transition.hpp"

#include "config/config.hpp"
#include "logging/logger.hpp"
#include "mathutil.hpp"

#include <cmath>

ui::Transition::Transition(int x, int y, int targetX, int targetY, int threshold) noexcept
    : m_x(x)
    , m_y(y)
    , m_targetX(targetX)
    , m_targetY(targetY)
    , m_threshold(threshold)
    , m_scaling(config::get_animation_scaling()) {};

void ui::Transition::update() noexcept
{
    ui::Transition::update_x_coord();
    ui::Transition::update_y_coord();
}

bool ui::Transition::in_place() const noexcept { return m_x == m_targetX && m_y == m_targetY; }

int ui::Transition::get_x() const noexcept { return static_cast<int>(m_x); }

int ui::Transition::get_y() const noexcept { return static_cast<int>(m_y); }

int ui::Transition::get_target_x() const noexcept { return static_cast<int>(m_targetX); }

int ui::Transition::get_target_y() const noexcept { return static_cast<int>(m_targetY); }

void ui::Transition::set_x(int x) noexcept { m_x = x; }

void ui::Transition::set_y(int y) noexcept { m_y = y; }

void ui::Transition::set_target_x(int targetX) noexcept { m_targetX = targetX; }

void ui::Transition::set_target_y(int targetY) noexcept { m_targetY = targetY; }

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
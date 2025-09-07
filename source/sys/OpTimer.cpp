#include "sys/OpTimer.hpp"

#include "logging/logger.hpp"

sys::OpTimer::OpTimer(const std::source_location &location) noexcept
    : m_location(location)
    , m_begin(std::chrono::high_resolution_clock::now()) {};

sys::OpTimer::~OpTimer() noexcept
{
    const auto end  = std::chrono::high_resolution_clock::now();
    const auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - m_begin);

    std::string_view functionName = OpTimer::get_function_name();
    logger::log("%s took %lli microseconds.", functionName.data(), diff.count());
}

std::string_view sys::OpTimer::get_function_name() const noexcept
{
    std::string_view function = m_location.function_name();
    const size_t nameBegin    = function.find_first_of(' ');
    if (nameBegin != function.npos) { function = function.substr(nameBegin + 1); }

    return function;
}
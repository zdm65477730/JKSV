#pragma once

#include <functional>
#include <memory>

namespace sys::threadpool
{
    // clang-format off
    struct DataStruct{};
    // clang-format on

    /// @brief Makes things easier to read and type later.
    using JobData = std::shared_ptr<sys::threadpool::DataStruct>;

    /// @brief Function definition.
    using JobFunction = std::function<void(JobData)>;

    /// @brief Initializes and starts the threads.
    void initialize();

    /// @brief Signals the threads to terminate and closes them.
    void exit();

    /// @brief Pushes a job to the queue.
    void push_job(sys::threadpool::JobFunction function, sys::threadpool::JobData);
}
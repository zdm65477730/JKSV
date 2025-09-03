#pragma once
#include "sys/Task.hpp"

#include <switch.h>

namespace sys
{
    class DataTask final : public sys::Task
    {
        public:
            DataTask(ThreadFunc function, bool clearCache);

            ~DataTask();

        private
            Thread m_thread{};
    }
}
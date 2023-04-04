/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSCHEDULER_HPP_INCLUDE
#define MOCKSCHEDULER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Scheduler.hpp"

class MockScheduler : public geopm::Scheduler
{
    public:
        MOCK_METHOD(int, num_cpu, (), (const, override));
        MOCK_METHOD(int, get_cpu, (), (const, override));
        MOCK_METHOD((std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >), proc_cpuset, (), (const, override));
        MOCK_METHOD((std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >), proc_cpuset, (int pid), (const, override));
        MOCK_METHOD((std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >), woomp, (int pid),  (const, override));
};

#endif

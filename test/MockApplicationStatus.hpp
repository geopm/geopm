/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKAPPLICATIONSTATUS_HPP_INCLUDE
#define MOCKAPPLICATIONSTATUS_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ApplicationStatus.hpp"

class MockApplicationStatus : public geopm::ApplicationStatus
{
    public:
        MOCK_METHOD(void, set_hint, (int cpu_idx, uint64_t hints), (override));
        MOCK_METHOD(uint64_t, get_hint, (int cpu_idx), (const, override));
        MOCK_METHOD(void, set_hash, (int cpu_idx, uint64_t hash, uint64_t hint),
                    (override));
        MOCK_METHOD(uint64_t, get_hash, (int cpu_idx), (const, override));
        MOCK_METHOD(void, reset_work_units, (int cpu_idx), (override));
        MOCK_METHOD(void, set_total_work_units, (int cpu_idx, int work_units),
                    (override));
        MOCK_METHOD(void, increment_work_unit, (int cpu_idx), (override));
        MOCK_METHOD(double, get_progress_cpu, (int cpu_idx), (const, override));
        MOCK_METHOD(void, set_valid_cpu, (const std::set<int> &cpu_idx), (override));
        MOCK_METHOD(void, update_cache, (), (override));
};

#endif

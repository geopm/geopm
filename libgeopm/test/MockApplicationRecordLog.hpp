/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKAPPLICATIONRECORDLOG_HPP_INCLUDE
#define MOCKAPPLICATIONRECORDLOG_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ApplicationRecordLog.hpp"

class MockApplicationRecordLog : public geopm::ApplicationRecordLog
{
    public:
        MOCK_METHOD(void, enter, (uint64_t hash, const geopm_time_s &time), (override));
        MOCK_METHOD(void, exit, (uint64_t hash, const geopm_time_s &time), (override));
        MOCK_METHOD(void, epoch, (const geopm_time_s &time), (override));
        MOCK_METHOD(void, dump,
                    (std::vector<geopm::record_s> & records,
                     std::vector<geopm::short_region_s> &short_regions),
                    (override));
        MOCK_METHOD(void, affinity, (const geopm_time_s &time, int cpu_idx), (override));
        MOCK_METHOD(void, cpuset_changed, (const geopm_time_s &time), (override));
        MOCK_METHOD(void, start_profile, (const geopm_time_s &time, const std::string &profile_name), (override));
        MOCK_METHOD(void, stop_profile, (const geopm_time_s &time, const std::string &profile_name), (override));
        MOCK_METHOD(void, overhead, (const geopm_time_s &time, double overhead_sec), (override));
};

#endif

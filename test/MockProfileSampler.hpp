/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKPROFILESAMPLER_HPP_INCLUDE
#define MOCKPROFILESAMPLER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Comm.hpp"
#include "ProfileSampler.hpp"

class MockProfileSampler : public geopm::ProfileSampler
{
    public:
        MOCK_METHOD(bool, do_shutdown, (), (const, override));
        MOCK_METHOD(bool, do_report, (), (const, override));
        MOCK_METHOD(void, region_names, (), (override));
        MOCK_METHOD(void, initialize, (), (override));
        MOCK_METHOD(int, rank_per_node, (), (const, override));
        MOCK_METHOD(std::vector<int>, cpu_rank, (), (const, override));
        MOCK_METHOD(std::set<std::string>, name_set, (), (const, override));
        MOCK_METHOD(std::string, report_name, (), (const, override));
        MOCK_METHOD(std::string, profile_name, (), (const, override));
        MOCK_METHOD(void, controller_ready, (), (override));
        MOCK_METHOD(void, abort, (), (override));
        MOCK_METHOD(void, check_sample_end, (), (override));
};

#endif

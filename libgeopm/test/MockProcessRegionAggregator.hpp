/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKPROCESSREGIONAGGREGATOR_HPP_INCLUDE
#define MOCKPROCESSREGIONAGGREGATOR_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ProcessRegionAggregator.hpp"

class MockProcessRegionAggregator : public geopm::ProcessRegionAggregator
{
    public:
        MOCK_METHOD(void, update, (), (override));
        MOCK_METHOD(double, get_runtime_average, (uint64_t region_hash),
                    (const, override));
        MOCK_METHOD(double, get_count_average, (uint64_t region_hash),
                    (const, override));
};

#endif

/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSTATSCOLLECTOR_HPP_INCLUDE
#define MOCKSTATSCOLLECTOR_HPP_INCLUDE

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "StatsCollector.hpp"

class MockStatsCollector : public geopm::StatsCollector
{
    public:
        MockStatsCollector() = default;
        virtual ~MockStatsCollector() = default;
        MOCK_METHOD(void, update, (), (override));
        MOCK_METHOD(std::string, report_yaml, (), (const, override));
        MOCK_METHOD(void, reset, (), (override));
        MOCK_METHOD(report_s, report_struct, (), (const, override));
        MOCK_METHOD(size_t, update_count, (), (const, override));
};

#endif

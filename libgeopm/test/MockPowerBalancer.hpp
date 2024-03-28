/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKPOWERBALANCER_HPP_INCLUDE
#define MOCKPOWERBALANCER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "PowerBalancer.hpp"

class MockPowerBalancer : public geopm::PowerBalancer
{
    public:
        MOCK_METHOD(void, power_cap, (double cap), (override));
        MOCK_METHOD(double, power_cap, (), (const, override));
        MOCK_METHOD(double, power_limit, (), (const, override));
        MOCK_METHOD(void, power_limit_adjusted, (double limit), (override));
        MOCK_METHOD(bool, is_runtime_stable, (double measured_runtime), (override));
        MOCK_METHOD(double, runtime_sample, (), (const, override));
        MOCK_METHOD(void, calculate_runtime_sample, (), (override));
        MOCK_METHOD(void, target_runtime, (double largest_runtime), (override));
        MOCK_METHOD(bool, is_target_met, (double measured_runtime), (override));
        MOCK_METHOD(double, power_slack, (), (override));
};

#endif

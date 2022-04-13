/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKPOWERGOVERNOR_HPP_INCLUDE
#define MOCKPOWERGOVERNOR_HPP_INCLUDE

#include "gmock/gmock.h"

#include "PowerGovernor.hpp"

class MockPowerGovernor : public geopm::PowerGovernor
{
    public:
        MOCK_METHOD(void, init_platform_io, (), (override));
        MOCK_METHOD(void, sample_platform, (), (override));
        MOCK_METHOD(void, adjust_platform,
                    (double node_power_request, double &node_power_actual),
                    (override));
        MOCK_METHOD(bool, do_write_batch, (), (const, override));
        MOCK_METHOD(void, set_power_bounds,
                    (double min_pkg_power, double max_pkg_power), (override));
        MOCK_METHOD(double, power_package_time_window, (), (const, override));
};

#endif

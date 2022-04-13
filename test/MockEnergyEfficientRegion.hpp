/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKENERGYEFFICIENTREGION_HPP_INCLUDE
#define MOCKENERGYEFFICIENTREGION_HPP_INCLUDE

#include "gmock/gmock.h"

#include "EnergyEfficientRegion.hpp"

class MockEnergyEfficientRegion : public geopm::EnergyEfficientRegion
{
    public:
        MOCK_METHOD(double, freq, (), (const, override));
        MOCK_METHOD(void, update_freq_range,
                    (double freq_min, double freq_max, double freq_step), (override));
        MOCK_METHOD(void, update_exit, (double curr_perf_metric), (override));
        MOCK_METHOD(bool, is_learning, (), (const, override));
};

#endif

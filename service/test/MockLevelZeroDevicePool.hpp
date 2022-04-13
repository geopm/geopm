/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKLEVELZERODEVICEPOOL_HPP_INCLUDE
#define MOCKLEVELZERODEVICEPOOL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "LevelZeroDevicePool.hpp"

class MockLevelZeroDevicePool : public geopm::LevelZeroDevicePool
{
    public:
        MOCK_METHOD(int, num_accelerator,
                    (int), (const, override));

        MOCK_METHOD(double, frequency_status,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, frequency_min,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, frequency_max,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD((std::pair<double, double>), frequency_range,
                    (int, unsigned int, int), (const, override));

        MOCK_METHOD((std::pair<uint64_t, uint64_t>), active_time_pair,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(uint64_t, active_time,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(uint64_t, active_time_timestamp,
                    (int, unsigned int, int), (const, override));

        MOCK_METHOD(int32_t, power_limit_tdp,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(int32_t, power_limit_min,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(int32_t, power_limit_max,
                    (int, unsigned int, int), (const, override));

        MOCK_METHOD((std::pair<uint64_t, uint64_t>), energy_pair,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(uint64_t, energy,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(uint64_t, energy_timestamp,
                    (int, unsigned int, int), (const, override));

        MOCK_METHOD(void, frequency_control,
                    (int, unsigned int, int, double, double),(const, override));
};

#endif

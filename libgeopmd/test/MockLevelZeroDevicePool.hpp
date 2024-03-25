/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKLEVELZERODEVICEPOOL_HPP_INCLUDE
#define MOCKLEVELZERODEVICEPOOL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "LevelZeroDevicePool.hpp"

class MockLevelZeroDevicePool : public geopm::LevelZeroDevicePool
{
    public:
        MOCK_METHOD(int, num_gpu,
                    (int), (const, override));

        MOCK_METHOD(double, ras_reset_count,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, ras_programming_errcount,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, ras_driver_errcount,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, ras_compute_errcount,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, ras_noncompute_errcount,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, ras_cache_errcount,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, ras_display_errcount,
                    (int, unsigned int, int), (const, override));

        MOCK_METHOD(double, frequency_status,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, frequency_efficient,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, frequency_min,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, frequency_max,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(double, frequency_step,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(uint32_t, frequency_throttle_reasons,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD((std::pair<double, double>), frequency_range,
                    (int, unsigned int, int), (const, override));

        MOCK_METHOD(double, temperature_max,
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

        MOCK_METHOD(double, performance_factor,
                    (int, unsigned int, int), (const, override));

        MOCK_METHOD(void, frequency_control,
                    (int, unsigned int, int, double, double),(const, override));
        MOCK_METHOD(void, performance_factor_control,
                    (int, unsigned int, int, double),(const, override));
};

#endif

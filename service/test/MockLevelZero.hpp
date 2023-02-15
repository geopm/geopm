/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKLEVELZERO_HPP_INCLUDE
#define MOCKLEVELZERO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "LevelZero.hpp"

class MockLevelZero : public geopm::LevelZero
{
    public:
        MOCK_METHOD(int, num_gpu, (), (const, override));

        MOCK_METHOD(int, num_gpu, (int), (const, override));

        MOCK_METHOD(int, frequency_domain_count, (unsigned int, int),
                    (const, override));
        MOCK_METHOD(double, frequency_status, (unsigned int, int, int),
                    (const, override));
        MOCK_METHOD(double, frequency_min, (unsigned int, int, int),
                    (const, override));
        MOCK_METHOD(double, frequency_max, (unsigned int, int, int),
                    (const, override));
        MOCK_METHOD(uint32_t, frequency_throttle_reasons, (unsigned int, int, int),
                    (const, override));
        MOCK_METHOD((std::pair<double, double>), frequency_range,
                    (unsigned int, int, int), (const, override));

        MOCK_METHOD(int, engine_domain_count, (unsigned int, int),
                    (const, override));
        MOCK_METHOD((std::pair<uint64_t, uint64_t>), active_time_pair,
                    (unsigned int, int, int),
                    (const, override));
        MOCK_METHOD(uint64_t, active_time,
                    (unsigned int, int, int), (const, override));
        MOCK_METHOD(uint64_t, active_time_timestamp,
                    (unsigned int, int, int), (const, override));

        MOCK_METHOD((std::pair<uint64_t, uint64_t>), energy_pair,
                    (unsigned int), (const, override));
        MOCK_METHOD(uint64_t, energy,
                    (unsigned int), (const, override));
        MOCK_METHOD(uint64_t, energy_timestamp,
                    (unsigned int), (const, override));
        MOCK_METHOD(int32_t, power_limit_tdp,
                    (unsigned int), (const, override));
        MOCK_METHOD(int32_t, power_limit_min,
                    (unsigned int), (const, override));
        MOCK_METHOD(int32_t, power_limit_max,
                    (unsigned int), (const, override));

        MOCK_METHOD(void, frequency_control,
                    (unsigned int, int, int, double, double), (const, override));
};

#endif

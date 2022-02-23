/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MOCKLEVELZERO_HPP_INCLUDE
#define MOCKLEVELZERO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "LevelZero.hpp"

class MockLevelZero : public geopm::LevelZero
{
    public:
        MOCK_METHOD(int, num_accelerator, (), (const, override));

        MOCK_METHOD(int, num_accelerator, (int), (const, override));

        MOCK_METHOD(int, frequency_domain_count, (unsigned int, int),
                    (const, override));
        MOCK_METHOD(double, frequency_status, (unsigned int, int, int),
                    (const, override));
        MOCK_METHOD(double, frequency_min, (unsigned int, int, int),
                    (const, override));
        MOCK_METHOD(double, frequency_max, (unsigned int, int, int),
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

        MOCK_METHOD(int, power_domain_count, (int, unsigned int, int),
                    (const, override));
        MOCK_METHOD((std::pair<uint64_t, uint64_t>), energy_pair,
                    (int, unsigned int, int), (const, override));
        MOCK_METHOD(uint64_t, energy,
                    (int, unsigned int, int, int), (const, override));
        MOCK_METHOD(uint64_t, energy_timestamp,
                    (int, unsigned int, int, int), (const, override));
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

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

#ifndef MOCKSAMPLEAGGREGATOR_HPP_INCLUDE
#define MOCKSAMPLEAGGREGATOR_HPP_INCLUDE

#include "gmock/gmock.h"

#include "SampleAggregator.hpp"

class MockSampleAggregator : public geopm::SampleAggregator
{
    public:
        MOCK_METHOD(int, push_signal_total,
                    (const std::string &signal_name, int domain_type, int domain_idx),
                    (override));
        MOCK_METHOD(int, push_signal_average,
                    (const std::string &signal_name, int domain_type, int domain_idx),
                    (override));
        MOCK_METHOD(int, push_signal,
                    (const std::string &signal_name, int domain_type, int domain_idx),
                    (override));
        MOCK_METHOD(void, update, (), (override));
        MOCK_METHOD(double, sample_application, (int signal_idx), (override));
        MOCK_METHOD(double, sample_epoch, (int signal_idx), (override));
        MOCK_METHOD(double, sample_region,
                    (int signal_idx, uint64_t region_hash), (override));
        MOCK_METHOD(double, sample_epoch_last, (int signal_idx), (override));
        MOCK_METHOD(double, sample_region_last,
                    (int signal_idx, uint64_t region_hash), (override));
        MOCK_METHOD(double, sample_period_last, (int signal_idx), (override));
        MOCK_METHOD(void, period_duration, (double), (override));
        MOCK_METHOD(int, get_period, (), (override));
};

#endif

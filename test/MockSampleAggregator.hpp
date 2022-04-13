/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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

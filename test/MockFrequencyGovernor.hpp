/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKFREQUENCYGOVERNOR_HPP_INCLUDE
#define MOCKFREQUENCYGOVERNOR_HPP_INCLUDE

#include "gmock/gmock.h"

#include "FrequencyGovernor.hpp"

class MockFrequencyGovernor : public geopm::FrequencyGovernor
{
    public:
        MOCK_METHOD(void, init_platform_io, (), (override));
        MOCK_METHOD(int, frequency_domain_type, (), (const, override));
        MOCK_METHOD(void, set_domain_type, (int domain_type), (override));
        MOCK_METHOD(void, adjust_platform,
                    (const std::vector<double> &frequency_request), (override));
        MOCK_METHOD(bool, do_write_batch, (), (const, override));
        MOCK_METHOD(bool, set_frequency_bounds,
                    (double freq_min, double freq_max), (override));
        MOCK_METHOD(double, get_frequency_min, (), (const, override));
        MOCK_METHOD(double, get_frequency_max, (), (const, override));
        MOCK_METHOD(double, get_frequency_step, (), (const, override));
        MOCK_METHOD(int, get_clamp_count, (), (const, override));
        MOCK_METHOD(void, validate_policy, (double &freq_min, double &freq_max),
                    (const, override));
};

#endif

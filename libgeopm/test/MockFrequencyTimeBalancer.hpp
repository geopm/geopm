/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKFREQUENCYTIMEBALANCER_HPP_INCLUDE
#define MOCKFREQUENCYTIMEBALANCER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "FrequencyTimeBalancer.hpp"

class MockFrequencyTimeBalancer : public geopm::FrequencyTimeBalancer
{
    public:
        MOCK_METHOD(std::vector<double>, balance_frequencies_by_time,
                    (const std::vector<double> &previous_times,
                     const std::vector<double> &previous_control_frequencies,
                     const std::vector<double> &previous_achieved_frequencies,
                     (const std::vector<std::pair<unsigned int, double> >) &frequency_limits_by_high_priority_count,
                     double low_priority_frequency),
                    (override));
        MOCK_METHOD(double, get_target_time, (), (const, override));
};

#endif /* MOCKFREQUENCYTIMEBALANCER_HPP_INCLUDE */

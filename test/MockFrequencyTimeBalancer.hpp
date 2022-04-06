/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
                    (const std::vector<double>& previous_times,
                     const std::vector<double>& previous_control_frequencies,
                     const std::vector<double>& previous_achieved_frequencies,
                     const std::vector<double>& previous_max_frequencies),
                    (override));
        MOCK_METHOD(std::vector<double>, get_target_times, (), (const, override));
        MOCK_METHOD(double, get_cutoff_frequency, (unsigned int core), (const, override));
};

#endif /* MOCKFREQUENCYTIMEBALANCER_HPP_INCLUDE */

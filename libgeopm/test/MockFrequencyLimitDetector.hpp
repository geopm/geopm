/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKFREQUENCYLIMITDETECTOR_HPP_INCLUDE
#define MOCKFREQUENCYLIMITDETECTOR_HPP_INCLUDE

#include "gmock/gmock.h"

#include "FrequencyLimitDetector.hpp"

class MockFrequencyLimitDetector : public geopm::FrequencyLimitDetector
{
    public:
        MOCK_METHOD(void, update_max_frequency_estimates,
                    (const std::vector<double> &observed_core_frequencies),
                    (override));
        MOCK_METHOD((std::vector<std::pair<unsigned int, double> >),
                    get_core_frequency_limits,
                    (unsigned int core_idx),
                    (const, override));
        MOCK_METHOD(double, get_core_low_priority_frequency,
                    (unsigned int core_idx),
                    (const, override));
};

#endif

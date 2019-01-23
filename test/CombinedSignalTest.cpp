/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include <cmath>
#include "gtest/gtest.h"

#include "CombinedSignal.hpp"
#include "Exception.hpp"
#include "Agg.hpp"

using geopm::CombinedSignal;
using geopm::DerivativeCombinedSignal;
using geopm::Exception;

TEST(CombinedSignalTest, sample_sum)
{
    CombinedSignal comb_signal;
    std::vector<double> values = {0.0};
    double result = comb_signal.sample(values);
    EXPECT_DOUBLE_EQ(0.0, result);

    values = {4.1, 5, -6, 7, 18};
    result = comb_signal.sample(values);
    EXPECT_DOUBLE_EQ(28.1, result);
}

TEST(CombinedSignalTest, sample_max)
{
    CombinedSignal comb_signal {geopm::Agg::max};
    std::vector<double> values = {0.0};
    double result = comb_signal.sample(values);
    EXPECT_DOUBLE_EQ(0.0, result);

    values = {4.1, 5, -6, 7, 18};
    result = comb_signal.sample(values);
    EXPECT_DOUBLE_EQ(18, result);
}

TEST(CombinedSignalTest, sample_flat_derivative)
{
    DerivativeCombinedSignal comb_signal;
    std::vector<double> values = {0};
#ifdef GEOPM_DEBUG
    ASSERT_EQ(1u, values.size());
    EXPECT_THROW(comb_signal.sample(values), Exception);
    values = {1, 2, 3, 4};
    ASSERT_EQ(4u, values.size());
    EXPECT_THROW(comb_signal.sample(values), Exception);
#endif

    // values expected: time, value
    values = {0, 5};
    double result = comb_signal.sample(values);
    EXPECT_TRUE(std::isnan(result));

    values = {1, 5};
    result = comb_signal.sample(values);
    EXPECT_DOUBLE_EQ(0.0, result);

    values = {2, 5};
    result = comb_signal.sample(values);
    EXPECT_DOUBLE_EQ(0.0, result);
}

TEST(CombinedSignalTest, sample_slope_derivative)
{
    DerivativeCombinedSignal comb_signal;
    // should have slope of 1.0
    std::vector<double> sample_values = {0.000001, .999999, 2.000001,
                                         2.999999, 4.000001, 4.999999,
                                         6.000001, 6.999999, 8.000001,
                                         8.999999};

    double result = NAN;
    for (size_t ii = 0; ii < sample_values.size(); ++ii) {
        result = comb_signal.sample({(double)ii, sample_values[ii]});
    }
    EXPECT_NEAR(1.0, result, 0.0001);

    // should have slope of .238 with least squares fit
    sample_values = {0, 1, 2, 3, 0, 1, 2, 3};
    for (size_t ii = 0; ii < sample_values.size(); ++ii) {
        result = comb_signal.sample({(double)ii, sample_values[ii]});
    }
    EXPECT_NEAR(0.238, result, 0.001);
}

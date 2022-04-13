/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cmath>
#include "gtest/gtest.h"

#include "CombinedSignal.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "config.h"

using geopm::CombinedSignal;
using geopm::DerivativeCombinedSignal;
using geopm::DifferenceCombinedSignal;
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

TEST(CombinedSignalTest, sample_difference)
{
    DifferenceCombinedSignal comb_signal;
    std::vector<double> values = {0};
#ifdef GEOPM_DEBUG
    ASSERT_EQ(1u, values.size());
    EXPECT_THROW(comb_signal.sample(values), Exception);
    values = {1, 2, 3, 4};
    ASSERT_EQ(4u, values.size());
    EXPECT_THROW(comb_signal.sample(values), Exception);
#endif

    values = {0, 5};
    double result = comb_signal.sample(values);
    EXPECT_DOUBLE_EQ(-5.0, result);

    values = {10, 5};
    result = comb_signal.sample(values);
    EXPECT_DOUBLE_EQ(5.0, result);
}

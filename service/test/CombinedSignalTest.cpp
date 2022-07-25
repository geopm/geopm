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

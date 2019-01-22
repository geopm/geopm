/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#include "geopm_test.hpp"

#include "Agg.hpp"
#include "geopm_internal.h"
#include "geopm_hash.h"

using geopm::Agg;

TEST(AggTest, agg_function)
{
    std::vector<double> data {16, 2, 4, 9, 128, 32, 4, 64};
    double sum = 259;
    double average = 32.375;
    double median = 12.5;
    double min = 2;
    double max = 128;
    double stddev = 43.902;
    EXPECT_DOUBLE_EQ(sum, Agg::sum(data));
    EXPECT_DOUBLE_EQ(average, Agg::average(data));
    EXPECT_DOUBLE_EQ(median, Agg::median(data));
    EXPECT_DOUBLE_EQ(4, Agg::median({4}));
    EXPECT_DOUBLE_EQ(4, Agg::median({2, 4, 6}));
    EXPECT_DOUBLE_EQ(min, Agg::min(data));
    EXPECT_DOUBLE_EQ(max, Agg::max(data));
    EXPECT_NEAR(stddev, Agg::stddev(data), 0.001);
    EXPECT_EQ(16.0, Agg::select_first(data));

    EXPECT_TRUE(std::isnan(Agg::expect_same({2.0, 2.0, 3.0, 2.0})));
    EXPECT_EQ(5.5, Agg::expect_same({5.5, 5.5, 5.5}));

    EXPECT_EQ(1.0, Agg::logical_and({1.0, 1.0}));
    EXPECT_EQ(0.0, Agg::logical_and({1.0, 1.0, 0.0}));
    EXPECT_EQ(1.0, Agg::logical_or({1.0, 1.0}));
    EXPECT_EQ(1.0, Agg::logical_or({1.0, 1.0, 0.0}));
    EXPECT_EQ(0.0, Agg::logical_or({0.0, 0.0}));

    EXPECT_EQ(geopm_field_to_signal(GEOPM_REGION_ID_UNMARKED),
              Agg::region_id({5, 6, 7}));
}

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

#include "geopm_test.hpp"

#include "geopm/Agg.hpp"
#include "geopm_hash.h"
#include "geopm_hint.h"
#include "geopm_test.hpp"

using geopm::Agg;

TEST(AggTest, agg_function)
{
    // NAN values will be ignored
    std::vector<double> data {16, 2, 4, NAN, 9, 128, NAN, 32, 4, 64};
    double sum = 259;
    double average = 32.375;
    double median = 12.5;
    double min = 2;
    double max = 128;
    double stddev = 43.902;
    EXPECT_DOUBLE_EQ(sum, Agg::sum(data));
    EXPECT_DOUBLE_EQ(average, Agg::average(data));
    EXPECT_DOUBLE_EQ(median, Agg::median(data));
    EXPECT_DOUBLE_EQ(4, Agg::median({4, NAN}));
    EXPECT_DOUBLE_EQ(4, Agg::median({2, 4, NAN, 6}));
    EXPECT_DOUBLE_EQ(min, Agg::min(data));
    EXPECT_DOUBLE_EQ(max, Agg::max(data));
    EXPECT_NEAR(stddev, Agg::stddev(data), 0.001);
    EXPECT_EQ(16.0, Agg::select_first(data));

    EXPECT_TRUE(std::isnan(Agg::expect_same({2.0, NAN, 2.0, 3.0, 2.0})));
    EXPECT_EQ(5.5, Agg::expect_same({5.5, 5.5, 5.5, NAN}));

    EXPECT_EQ(1.0, Agg::logical_and({1.0, NAN, 1.0}));
    EXPECT_EQ(0.0, Agg::logical_and({1.0, 1.0, 0.0, NAN}));
    EXPECT_EQ(1.0, Agg::logical_or({1.0, NAN, 1.0}));
    EXPECT_EQ(1.0, Agg::logical_or({1.0, 1.0, 0.0}));
    EXPECT_EQ(0.0, Agg::logical_or({0.0, 0.0}));

    EXPECT_TRUE(std::isnan(Agg::region_hash({})));

    EXPECT_TRUE(std::isnan(Agg::region_hash({NAN, NAN})));

    EXPECT_EQ(GEOPM_REGION_HASH_UNMARKED,
              Agg::region_hash({5, 6, NAN, 7}));

    EXPECT_EQ(5,
              Agg::region_hash({5, 5, 5, NAN}));

    EXPECT_TRUE(std::isnan(Agg::region_hint({})));

    EXPECT_TRUE(std::isnan(Agg::region_hint({NAN, NAN})));

    EXPECT_EQ(GEOPM_REGION_HINT_UNKNOWN,
              Agg::region_hint({5, 6, NAN, 7}));

    EXPECT_EQ(5,
              Agg::region_hint({5, 5, 5, NAN}));
}

TEST(AggTest, function_strings)
{
    EXPECT_TRUE(is_agg_sum(Agg::name_to_function("sum")));
    EXPECT_TRUE(is_agg_average(Agg::name_to_function("average")));
    EXPECT_TRUE(is_agg_median(Agg::name_to_function("median")));
    EXPECT_TRUE(is_agg_logical_and(Agg::name_to_function("logical_and")));
    EXPECT_TRUE(is_agg_logical_or(Agg::name_to_function("logical_or")));
    EXPECT_TRUE(is_agg_region_hash(Agg::name_to_function("region_hash")));
    EXPECT_TRUE(is_agg_region_hint(Agg::name_to_function("region_hint")));
    EXPECT_TRUE(is_agg_min(Agg::name_to_function("min")));
    EXPECT_TRUE(is_agg_max(Agg::name_to_function("max")));
    EXPECT_TRUE(is_agg_stddev(Agg::name_to_function("stddev")));
    EXPECT_TRUE(is_agg_select_first(Agg::name_to_function("select_first")));
    EXPECT_TRUE(is_agg_expect_same(Agg::name_to_function("expect_same")));

    GEOPM_EXPECT_THROW_MESSAGE(Agg::name_to_function("invalid"), GEOPM_ERROR_INVALID,
                               "unknown aggregation function");
}

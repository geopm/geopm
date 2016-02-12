/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <iostream>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "Region.hpp"

class RegionTest: public :: testing :: Test
{
    protected:
        void SetUp();
        geopm::Region *m_leaf_region;
        geopm::Region *m_tree_region;
};

void RegionTest::SetUp()
{
    int i, j, k;

    m_leaf_region = new geopm::Region(42, GEOPM_POLICY_HINT_COMPUTE, 2, 0);
    m_tree_region = new geopm::Region(42, GEOPM_POLICY_HINT_COMPUTE, 8, 1);

    std::vector<struct geopm_telemetry_message_s> telemetry(2);
    std::vector<struct geopm_sample_message_s> sample(8);

    for (int sample_idx = 0; sample_idx < 8; ++sample_idx) {
        if (sample_idx < 2) {
            telemetry[sample_idx].region_id = 42;
        }
        sample[sample_idx].region_id = 42;
    }
    for (i = 0; i < 5; ++i) {
        geopm_time(&telemetry[0].timestamp);
        telemetry[1].timestamp = telemetry[0].timestamp;
        for (j = 0; j < GEOPM_NUM_TELEMETRY_TYPE; j++) {
            telemetry[0].signal[j] = (double)i;
            telemetry[1].signal[j] = (double)(i+5);
            if (j < GEOPM_NUM_SAMPLE_TYPE) {
                for (k = 0; k < 8; ++k) {
                    sample[k].signal[j] = (double)(i + k);
                }
            }
        }
        m_leaf_region->insert(telemetry);
        m_tree_region->insert(sample);
    }
}

TEST_F(RegionTest, signal_mean)
{
    EXPECT_DOUBLE_EQ(2.0, m_leaf_region->mean(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(7.0, m_leaf_region->mean(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
}

TEST_F(RegionTest, signal_median)
{
    EXPECT_DOUBLE_EQ(2.0, m_leaf_region->median(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(7.0, m_leaf_region->median(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
}

TEST_F(RegionTest, signal_stddev)
{
    EXPECT_DOUBLE_EQ(1.4142135623730951, m_leaf_region->std_deviation(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(1.4142135623730951, m_leaf_region->std_deviation(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
}

TEST_F(RegionTest, signal_max)
{
    EXPECT_DOUBLE_EQ(4.0, m_leaf_region->max(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(9.0, m_leaf_region->max(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
}

TEST_F(RegionTest, signal_min)
{
    EXPECT_DOUBLE_EQ(0.0, m_leaf_region->min(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(5.0, m_leaf_region->min(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
}

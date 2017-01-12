/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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
        struct geopm_time_s m_time;
};

void RegionTest::SetUp()
{
    int i, j, k;

    geopm_time(&m_time);

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
    for (i = 0; i < 7; ++i) {
        m_time.t.tv_sec += 2;
        telemetry[0].timestamp = m_time;
        telemetry[1].timestamp = m_time;
        for (j = 0; j < GEOPM_NUM_TELEMETRY_TYPE; j++) {
            if (j == GEOPM_TELEMETRY_TYPE_PROGRESS) {
                telemetry[0].signal[j] = (double)i/8.0;
                telemetry[1].signal[j] = (double)i/8.0;
            }
            else {
                telemetry[0].signal[j] = (double)i;
                telemetry[1].signal[j] = (double)(i+5);
            }
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

TEST_F(RegionTest, identifier)
{
    EXPECT_EQ((uint64_t)42, m_leaf_region->identifier());
    EXPECT_EQ((uint64_t)42, m_tree_region->identifier());
}

TEST_F(RegionTest, hint)
{
    EXPECT_EQ(GEOPM_POLICY_HINT_COMPUTE, m_leaf_region->hint());
    EXPECT_EQ(GEOPM_POLICY_HINT_COMPUTE, m_tree_region->hint());
}

TEST_F(RegionTest, sample_message)
{
    struct geopm_sample_message_s sample;
    std::vector<struct geopm_telemetry_message_s> telemetry(2);
    telemetry[0].region_id = 42;
    telemetry[1].region_id = 42;
    // Add an entry from a new region
    m_time.t.tv_sec += 2;
    telemetry[0].timestamp = m_time;
    telemetry[1].timestamp = m_time;
    for (int j = 0; j < GEOPM_NUM_TELEMETRY_TYPE; j++) {
        if (j == GEOPM_TELEMETRY_TYPE_PROGRESS) {
            telemetry[0].signal[j] = 1.0;
            telemetry[1].signal[j] = 1.0;
        }
        else {
            telemetry[0].signal[j] = 7.0;
            telemetry[1].signal[j] = 12.0;
        }
    }
    m_leaf_region->insert(telemetry);

    m_leaf_region->sample_message(sample);
    EXPECT_EQ((uint64_t)42, sample.region_id);
    EXPECT_DOUBLE_EQ(14.0, sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME]);
    EXPECT_DOUBLE_EQ(28.0, sample.signal[GEOPM_SAMPLE_TYPE_ENERGY]);
    EXPECT_DOUBLE_EQ(14.0, sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_NUMER]);

    m_tree_region->sample_message(sample);
    EXPECT_EQ((uint64_t)42, sample.region_id);
    EXPECT_DOUBLE_EQ(13.0, sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME]);
    EXPECT_DOUBLE_EQ(76.0, sample.signal[GEOPM_SAMPLE_TYPE_ENERGY]);
    EXPECT_DOUBLE_EQ(76.0, sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_NUMER]);
}

TEST_F(RegionTest, signal_last)
{
    EXPECT_DOUBLE_EQ(6.0, m_leaf_region->signal(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(11.0, m_leaf_region->signal(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    for (int i = 0; i < 8; ++i) {
        EXPECT_DOUBLE_EQ(i + 6, m_tree_region->signal(i, GEOPM_SAMPLE_TYPE_RUNTIME));
    }
}

TEST_F(RegionTest, signal_num)
{
    EXPECT_EQ(7, m_leaf_region->num_sample(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_EQ(7, m_leaf_region->num_sample(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(7, m_tree_region->num_sample(i, GEOPM_SAMPLE_TYPE_RUNTIME));
    }
}

TEST_F(RegionTest, signal_derivative)
{
    EXPECT_DOUBLE_EQ(0.5, m_leaf_region->derivative(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(0.5, m_leaf_region->derivative(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
}

TEST_F(RegionTest, signal_mean)
{
    EXPECT_DOUBLE_EQ(3.0, m_leaf_region->mean(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(8.0, m_leaf_region->mean(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    for (int i = 0; i < 8; ++i) {
        EXPECT_DOUBLE_EQ(i + 3, m_tree_region->mean(i, GEOPM_SAMPLE_TYPE_RUNTIME));
    }
}

TEST_F(RegionTest, signal_median)
{
    EXPECT_DOUBLE_EQ(3.0, m_leaf_region->median(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(8.0, m_leaf_region->median(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    for (int i = 0; i < 8; ++i) {
        EXPECT_DOUBLE_EQ(i + 3, m_tree_region->median(i, GEOPM_SAMPLE_TYPE_RUNTIME));
    }
}

TEST_F(RegionTest, signal_stddev)
{
    EXPECT_DOUBLE_EQ(2.0, m_leaf_region->std_deviation(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(2.0, m_leaf_region->std_deviation(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    for (int i = 0; i < 8; ++i) {
        EXPECT_DOUBLE_EQ(2.0, m_tree_region->std_deviation(i, GEOPM_SAMPLE_TYPE_RUNTIME));
    }
}

TEST_F(RegionTest, signal_max)
{
    EXPECT_DOUBLE_EQ(6.0, m_leaf_region->max(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(11.0, m_leaf_region->max(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    for (int i = 0; i < 8; ++i) {
        EXPECT_DOUBLE_EQ((double)(i + 6), m_tree_region->max(i, GEOPM_SAMPLE_TYPE_RUNTIME));
    }
}

TEST_F(RegionTest, signal_min)
{
    EXPECT_DOUBLE_EQ(0.0, m_leaf_region->min(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(5.0, m_leaf_region->min(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    for (int i = 0; i < 8; ++i) {
        EXPECT_DOUBLE_EQ((double)i, m_tree_region->min(i, GEOPM_SAMPLE_TYPE_RUNTIME));
    }
}

TEST_F(RegionTest, signal_capacity_leaf)
{
    std::vector<struct geopm_telemetry_message_s> telemetry(2);
    for (int i = 0; i < 2; ++i) {
        telemetry[0].region_id = 42;
        telemetry[1].region_id = 42;
        geopm_time(&telemetry[0].timestamp);
        telemetry[1].timestamp = telemetry[0].timestamp;
        for (int j = 0; j < GEOPM_NUM_TELEMETRY_TYPE; j++) {
            telemetry[0].signal[j] = (double)(i+7);
            telemetry[1].signal[j] = (double)(i+12);
        }
        m_leaf_region->insert(telemetry);
    }
    EXPECT_DOUBLE_EQ(4.5, m_leaf_region->mean(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(9.5, m_leaf_region->mean(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(5.0, m_leaf_region->median(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(10.0, m_leaf_region->median(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(2.2912878474779199, m_leaf_region->std_deviation(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(2.2912878474779199, m_leaf_region->std_deviation(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(1.0, m_leaf_region->min(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(6.0, m_leaf_region->min(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(8.0, m_leaf_region->max(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(13.0, m_leaf_region->max(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(8.0, m_leaf_region->signal(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(13.0, m_leaf_region->signal(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_EQ(8, m_leaf_region->num_sample(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_EQ(8, m_leaf_region->num_sample(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
}

TEST_F(RegionTest, signal_capacity_tree)
{
    std::vector<struct geopm_sample_message_s> sample(8);
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 8; ++j) {
            sample[j].region_id = 42;
            for (int k = 0; k < GEOPM_NUM_SAMPLE_TYPE; ++k) {
                sample[j].signal[k] = (double)(i + (j + 7));
            }
        }
        m_tree_region->insert(sample);
    }

    for (int i = 0; i < 8; ++i) {
        EXPECT_DOUBLE_EQ((double)(i + 4.5), m_tree_region->mean(i, GEOPM_SAMPLE_TYPE_RUNTIME));
        EXPECT_DOUBLE_EQ((double)(i + 5.0), m_tree_region->median(i, GEOPM_SAMPLE_TYPE_RUNTIME));
        EXPECT_DOUBLE_EQ(2.2912878474779199, m_tree_region->std_deviation(i, GEOPM_SAMPLE_TYPE_RUNTIME));
        EXPECT_DOUBLE_EQ((double)(i + 1.0), m_tree_region->min(i, GEOPM_SAMPLE_TYPE_RUNTIME));
        EXPECT_DOUBLE_EQ((double)(i + 8.0), m_tree_region->max(i, GEOPM_SAMPLE_TYPE_RUNTIME));
        EXPECT_DOUBLE_EQ(i + 8.0, m_tree_region->signal(i, GEOPM_SAMPLE_TYPE_RUNTIME));
        EXPECT_EQ(8, m_tree_region->num_sample(i, GEOPM_SAMPLE_TYPE_RUNTIME));
    }
}

TEST_F(RegionTest, signal_invalid_entry)
{
    std::vector<struct geopm_telemetry_message_s> telemetry(2);
    telemetry[0].region_id = 42;
    telemetry[1].region_id = 42;
    // Add an invalid entry
    geopm_time(&telemetry[0].timestamp);
    telemetry[1].timestamp = telemetry[0].timestamp;
    for (int j = 0; j < GEOPM_NUM_TELEMETRY_TYPE; j++) {
        telemetry[0].signal[j] = -1.0;
        telemetry[1].signal[j] = -1.0;
    }
    m_leaf_region->insert(telemetry);
    EXPECT_DOUBLE_EQ(3.0, m_leaf_region->mean(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(8.0, m_leaf_region->mean(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(3.0, m_leaf_region->median(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(8.0, m_leaf_region->median(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(2.0, m_leaf_region->std_deviation(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(2.0, m_leaf_region->std_deviation(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(0.0, m_leaf_region->min(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(5.0, m_leaf_region->min(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(6.0, m_leaf_region->max(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(11.0, m_leaf_region->max(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(6.0, m_leaf_region->signal(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(11.0, m_leaf_region->signal(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_EQ(7, m_leaf_region->num_sample(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_EQ(7, m_leaf_region->num_sample(1, GEOPM_TELEMETRY_TYPE_RUNTIME));

    // Add another invalid entry. This one pushes a valid entry out of the buffer.
    geopm_time(&telemetry[0].timestamp);
    telemetry[1].timestamp = telemetry[0].timestamp;
    m_leaf_region->insert(telemetry);
    EXPECT_DOUBLE_EQ(3.5, m_leaf_region->mean(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(8.5, m_leaf_region->mean(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(4.0, m_leaf_region->median(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(9.0, m_leaf_region->median(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(1.707825127659933, m_leaf_region->std_deviation(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(1.7078251276599345, m_leaf_region->std_deviation(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(1.0, m_leaf_region->min(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(6.0, m_leaf_region->min(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(6.0, m_leaf_region->max(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(11.0, m_leaf_region->max(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(6.0, m_leaf_region->signal(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(11.0, m_leaf_region->signal(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_EQ(6, m_leaf_region->num_sample(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_EQ(6, m_leaf_region->num_sample(1, GEOPM_TELEMETRY_TYPE_RUNTIME));

    // Add valid entries untill we push one of the invalid entries out of the buffer.
    geopm_time(&telemetry[0].timestamp);
    telemetry[1].timestamp = telemetry[0].timestamp;
    for (int i = 0; i < 7; ++i) {
        for (int j = 0; j < GEOPM_NUM_TELEMETRY_TYPE; j++) {
            telemetry[0].signal[j] = (double)i;
            telemetry[1].signal[j] = (double)(i + 5.0);
        }
        m_leaf_region->insert(telemetry);
    }
    EXPECT_DOUBLE_EQ(3.0, m_leaf_region->mean(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(8.0, m_leaf_region->mean(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(3.0, m_leaf_region->median(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(8.0, m_leaf_region->median(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(2.0, m_leaf_region->std_deviation(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(2.0, m_leaf_region->std_deviation(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(0.0, m_leaf_region->min(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(5.0, m_leaf_region->min(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(6.0, m_leaf_region->max(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(11.0, m_leaf_region->max(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(6.0, m_leaf_region->signal(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_DOUBLE_EQ(11.0, m_leaf_region->signal(1, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_EQ(7, m_leaf_region->num_sample(0, GEOPM_TELEMETRY_TYPE_RUNTIME));
    EXPECT_EQ(7, m_leaf_region->num_sample(1, GEOPM_TELEMETRY_TYPE_RUNTIME));

}

TEST_F(RegionTest, negative_region_invalid)
{
    int thrown = 0;
    try {
        m_tree_region->mean(2, GEOPM_TELEMETRY_TYPE_RUNTIME);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->median(2, GEOPM_TELEMETRY_TYPE_RUNTIME);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->std_deviation(2, GEOPM_TELEMETRY_TYPE_RUNTIME);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->min(2, GEOPM_TELEMETRY_TYPE_RUNTIME);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->max(2, GEOPM_TELEMETRY_TYPE_RUNTIME);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->derivative(2, GEOPM_TELEMETRY_TYPE_RUNTIME);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->num_sample(2, GEOPM_TELEMETRY_TYPE_RUNTIME);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->signal(2, GEOPM_TELEMETRY_TYPE_RUNTIME);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
}

TEST_F(RegionTest, negative_signal_invalid)
{
    int thrown = 0;
    try {
        m_tree_region->mean(0, GEOPM_NUM_TELEMETRY_TYPE + 1);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->median(0, GEOPM_NUM_TELEMETRY_TYPE + 1);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->std_deviation(0, GEOPM_NUM_TELEMETRY_TYPE + 1);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->min(0, GEOPM_NUM_TELEMETRY_TYPE + 1);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->max(0, GEOPM_NUM_TELEMETRY_TYPE + 1);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->derivative(0, GEOPM_NUM_TELEMETRY_TYPE + 1);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->num_sample(0, GEOPM_NUM_TELEMETRY_TYPE + 1);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_tree_region->signal(0, GEOPM_NUM_TELEMETRY_TYPE + 1);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
}

TEST_F(RegionTest, negative_signal_derivative_tree)
{
    int thrown = 0;
    try {
        m_tree_region->derivative(0, GEOPM_SAMPLE_TYPE_RUNTIME);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_NOT_IMPLEMENTED, thrown);
}


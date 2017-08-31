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

#include "gtest/gtest.h"
#include "SampleRegulator.hpp"
#include "CircularBuffer.hpp"

class SampleRegulatorTest : public geopm::SampleRegulator, public testing::Test
{
    public:
        SampleRegulatorTest();
        virtual ~SampleRegulatorTest();
    protected:
        struct geopm_time_s m_test_sample_time[2];
        std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > m_test_prof;
        std::vector<int> m_test_cpu_rank;
        std::vector<double> m_test_plat;
};
// In this test we simulate 2 sockets (domains of control).  Each
// socket has 4 CPUs, there are 4 ranks (1, 2, ..., 4) with compact
// affinity over the 8 total CPUs.  There are three platform signals
// per cpu.
SampleRegulatorTest::SampleRegulatorTest(void)
    : SampleRegulator({1, 1, 2, 2, 3, 3, 4, 4})
    , m_test_cpu_rank({1, 1, 2, 2, 3, 3, 4, 4})
    , m_test_plat(24)
{
    struct geopm_prof_message_s msg;
    geopm_time(m_test_sample_time);
    geopm_time_add(m_test_sample_time, 1.0, m_test_sample_time + 1);

    msg.region_id = 42;
    msg.timestamp = m_test_sample_time[0];
    msg.progress = 0.1;
    for (int rank = 1; rank != 5; ++rank) {
        msg.rank = rank;
        m_test_prof.push_back(std::pair<uint64_t, struct geopm_prof_message_s>(msg.region_id, msg));
    }
    msg.timestamp = m_test_sample_time[1];
    msg.progress = 0.2;
    for (int rank = 1; rank != 5; ++rank) {
        msg.rank = rank;
        m_test_prof.push_back(std::pair<uint64_t, struct geopm_prof_message_s>(msg.region_id, msg));
    }

    for (unsigned i = 0; i != m_test_plat.size(); ++i) {
        m_test_plat[i] = i * i;
    }
}

SampleRegulatorTest::~SampleRegulatorTest()
{

}

TEST_F(SampleRegulatorTest, insert_platform)
{
    insert(m_test_prof.begin(), m_test_prof.end());
    insert(m_test_plat.begin(), m_test_plat.end());

    unsigned i = 0;
    for ( ; i < m_aligned_signal.size() - 8; ++i) {
        ASSERT_DOUBLE_EQ(i * i, m_aligned_signal[i]);
    }
    for ( ; i < m_aligned_signal.size(); ++i) {
        ASSERT_DOUBLE_EQ(0.0, m_aligned_signal[i]);
    }
}

TEST_F(SampleRegulatorTest, insert_profile)
{
    struct m_rank_sample_s sample;
    struct m_rank_sample_s sample_expect;

    insert(m_test_prof.begin(), m_test_prof.end());
    sample_expect.timestamp = m_test_sample_time[0];
    sample_expect.runtime = 0.0;
    sample_expect.progress = 0.1;
    for (int i = 0; i != 4; ++i) {
        ASSERT_EQ(m_rank_sample_prev[i]->size(), 2);
        sample = m_rank_sample_prev[i]->value(0);
        ASSERT_DOUBLE_EQ(0.0, geopm_time_diff(&(sample_expect.timestamp), &(sample.timestamp)));
        ASSERT_DOUBLE_EQ(sample_expect.progress, sample.progress);
        ASSERT_DOUBLE_EQ(sample_expect.runtime, sample.runtime);
    }

    sample_expect.timestamp = m_test_sample_time[1];
    sample_expect.progress = 0.2;
    sample_expect.runtime = 0.0;
    for (int i = 0; i != 4; ++i) {
        sample = m_rank_sample_prev[i]->value(1);
        ASSERT_DOUBLE_EQ(0.0, geopm_time_diff(&(sample_expect.timestamp), &(sample.timestamp)));
        ASSERT_DOUBLE_EQ(sample_expect.progress, sample.progress);
        ASSERT_DOUBLE_EQ(sample_expect.runtime, sample.runtime);
    }
}

TEST_F(SampleRegulatorTest, align_profile)
{
    // test when no profile data has been entered
    insert(m_test_prof.begin(), m_test_prof.begin());
    insert(m_test_plat.begin(), m_test_plat.end());
    align(m_test_sample_time[1]);
    for (unsigned i = 24; i < m_aligned_signal.size(); ++i) {
        if (i % 2) {
            ASSERT_DOUBLE_EQ(-1.0, m_aligned_signal[i]);
        }
        else {
            ASSERT_DOUBLE_EQ(0.0, m_aligned_signal[i]);
        }
    }

    // insert two profile samples and use last profile sample time
    insert(m_test_prof.begin(), m_test_prof.end());
    align(m_test_sample_time[1]);
    for (unsigned i = 24; i < m_aligned_signal.size(); ++i) {
        if (i % 2) { // runtime signal
            ASSERT_DOUBLE_EQ(0.0, m_aligned_signal[i]);
        }
        else { // progress signal
            ASSERT_DOUBLE_EQ(0.2, m_aligned_signal[i]);
        }
    }
    // extrapolate one second
    struct geopm_time_s platform_time;
    geopm_time_add(m_test_sample_time + 1, 1.0, &platform_time);
    align(platform_time);
    for (unsigned i = 24; i < m_aligned_signal.size(); ++i) {
        if (i % 2) { // runtime signal
            ASSERT_DOUBLE_EQ(0.0, m_aligned_signal[i]);
        }
        else { // progress signal
            ASSERT_DOUBLE_EQ(0.3, m_aligned_signal[i]);
        }
    }
    // extrapolate 100 seconds
    geopm_time_add(m_test_sample_time + 1, 100.0, &platform_time);
    align(platform_time);
    for (unsigned i = 24; i < m_aligned_signal.size(); ++i) {
        if (i % 2) { // runtime signal
            ASSERT_DOUBLE_EQ(0.0, m_aligned_signal[i]);
        }
        else { // progress signal
            ASSERT_NEAR(1.0, m_aligned_signal[i], 1E-9);
        }
    }
    // Give negative derivative
    m_test_prof[4].second.progress = 0.01;
    insert(m_test_prof.begin(), m_test_prof.end());
    align(platform_time);
    ASSERT_DOUBLE_EQ(0.01, m_aligned_signal[24]);

    // Test nearest sampling
    m_test_prof.resize(4);
    geopm_time_add(m_test_sample_time + 1, 8.0, &platform_time);
    for (auto it = m_test_prof.begin(); it != m_test_prof.end(); ++it) {
        ++(*it).second.region_id; // enter new region on all ranks
        (*it).second.progress = 0.4;
        (*it).second.timestamp = platform_time;
    }
    insert(m_test_prof.begin(), m_test_prof.end());
    geopm_time_add(m_test_sample_time + 1, 9.0, &platform_time);
    align(platform_time);
    for (unsigned i = 24; i < m_aligned_signal.size(); ++i) {
        if (i % 2) { // runtime signal
            ASSERT_DOUBLE_EQ(0.0, m_aligned_signal[i]);
        }
        else { // progress signal
            ASSERT_DOUBLE_EQ(0.4, m_aligned_signal[i]);
        }
    }
}

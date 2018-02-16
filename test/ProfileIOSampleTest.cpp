/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "geopm_time.h"
#include "ProfileIOSample.hpp"

class ProfileIOSampleTest : public ::testing::Test
{
    public:
        ProfileIOSampleTest();
    protected:
        std::vector<int> m_rank;
        geopm::ProfileIOSample m_profile_sample;
};

ProfileIOSampleTest::ProfileIOSampleTest()
    : m_rank{1, 1, 2, 2, 3, 3, 4, 4}
    , m_profile_sample(m_rank)
{

}

TEST_F(ProfileIOSampleTest, hello)
{
    geopm_time_s time_0;
    geopm_time(&time_0);
    std::vector<double> progress = m_profile_sample.per_cpu_progress(time_0);
    EXPECT_EQ(m_rank.size(), progress.size());
    for (const auto &it : progress) {
        EXPECT_EQ(0.0, it);
    }
    std::vector<uint64_t> region_id = m_profile_sample.per_cpu_region_id();
    for (const auto &it : region_id) {
        EXPECT_EQ(GEOPM_REGION_ID_UNMARKED, it);
    }

    std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > prof_sample;
    std::pair<uint64_t, struct geopm_prof_message_s> samp {42, {.rank=2, .region_id=42, .timestamp=time_0, .progress=0.5}};
    prof_sample.emplace_back(samp);
    m_profile_sample.update(prof_sample.begin(), prof_sample.end());

    progress = m_profile_sample.per_cpu_progress(time_0);
    region_id = m_profile_sample.per_cpu_region_id();

    for (size_t cpu_idx = 0; cpu_idx != m_rank.size(); ++cpu_idx) {
        if (cpu_idx == 2 || cpu_idx == 3) {
            EXPECT_EQ(0.5, progress[cpu_idx]);
            EXPECT_EQ(42ULL, region_id[cpu_idx]);
        }
        else {
            EXPECT_EQ(0.0, progress[cpu_idx]);
            EXPECT_EQ(GEOPM_REGION_ID_UNMARKED, region_id[cpu_idx]);
        }
    }

    struct geopm_time_s time_1;
    geopm_time_add(&time_0, 1.0, &time_1);
    progress = m_profile_sample.per_cpu_progress(time_1);
    region_id = m_profile_sample.per_cpu_region_id();

    // With one sample we will use nearest neighbor interplation
    for (size_t cpu_idx = 0; cpu_idx != m_rank.size(); ++cpu_idx) {
        if (cpu_idx == 2 || cpu_idx == 3) {
            EXPECT_EQ(0.5, progress[cpu_idx]);
            EXPECT_EQ(42ULL, region_id[cpu_idx]);
        }
        else {
            EXPECT_EQ(0.0, progress[cpu_idx]);
            EXPECT_EQ(GEOPM_REGION_ID_UNMARKED, region_id[cpu_idx]);
        }
    }

    prof_sample[0] = {42ULL, {.rank=2, .region_id=42ULL, .timestamp=time_1, .progress=0.6}};
    m_profile_sample.update(prof_sample.begin(), prof_sample.end());

    progress = m_profile_sample.per_cpu_progress(time_1);
    region_id = m_profile_sample.per_cpu_region_id();

    // With one sample we will use nearest neighbor interplation
    for (size_t cpu_idx = 0; cpu_idx != m_rank.size(); ++cpu_idx) {
        if (cpu_idx == 2 || cpu_idx == 3) {
            EXPECT_EQ(0.6, progress[cpu_idx]);
            EXPECT_EQ(42ULL, region_id[cpu_idx]);
        }
        else {
            EXPECT_EQ(0.0, progress[cpu_idx]);
            EXPECT_EQ(GEOPM_REGION_ID_UNMARKED, region_id[cpu_idx]);
        }
    }

    struct geopm_time_s time_2;
    geopm_time_add(&time_1, 1.0, &time_2);
    progress = m_profile_sample.per_cpu_progress(time_2);
    region_id = m_profile_sample.per_cpu_region_id();

    // With one sample we will use nearest neighbor interplation
    for (size_t cpu_idx = 0; cpu_idx != m_rank.size(); ++cpu_idx) {
        if (cpu_idx == 2 || cpu_idx == 3) {
            EXPECT_EQ(0.7, progress[cpu_idx]);
            EXPECT_EQ(42ULL, region_id[cpu_idx]);
        }
        else {
            EXPECT_EQ(0.0, progress[cpu_idx]);
            EXPECT_EQ(GEOPM_REGION_ID_UNMARKED, region_id[cpu_idx]);
        }
    }
}


// TODO test list
// update changes result of per_cpu_region_id
// update changes result of per_cpu_progress
// progress of 100% changes region id to GEOPM_REGION_ID_UNMARKED

// look at sample regulator tests

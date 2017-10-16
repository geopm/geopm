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

#include <stdlib.h>
#include <iostream>
#include <map>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_hash.h"
#include "DeciderFactory.hpp"

#include "Decider.hpp"
#include "SimpleFreqDecider.hpp"

#include "MockRegion.hpp"
#include "MockPolicy.hpp"
#include "geopm.h"

using  ::testing::_;
using  ::testing::Invoke;
using  ::testing::Sequence;
using  ::testing::Return;

class SimpleFreqDeciderTest: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        static const size_t M_NUM_REGIONS = 5;
        std::vector<size_t> m_hints;
        std::vector<double> m_expected_freqs;
        geopm::IDecider *m_decider;
        geopm::DeciderFactory *m_fact;
        MockRegion *m_mockregion;
        MockPolicy *m_mockpolicy;
        std::vector<std::string> m_region_names;
        std::vector<double> m_mapped_freqs;
        double m_freq_min;
        double m_freq_max;
};

void SimpleFreqDeciderTest::SetUp()
{
    setenv("GEOPM_PLUGIN_PATH", ".libs/", 1);

    m_freq_min = 1800000000.0;
    m_freq_max = 2200000000.0;
    m_region_names = {"mapped_region0", "mapped_region1", "mapped_region2", "mapped_region3", "mapped_region4"};
    m_mapped_freqs = {m_freq_max, 2100000000.0, 2000000000.0, 1900000000.0, m_freq_min};

    std::vector<size_t> m_hints = {GEOPM_REGION_HINT_UNKNOWN, GEOPM_REGION_HINT_COMPUTE, GEOPM_REGION_HINT_MEMORY,
               GEOPM_REGION_HINT_NETWORK, GEOPM_REGION_HINT_IO, GEOPM_REGION_HINT_SERIAL,
               GEOPM_REGION_HINT_PARALLEL, GEOPM_REGION_HINT_IGNORE};
    std::vector<double> m_expected_freqs = {m_freq_min, m_freq_max, m_freq_min, m_freq_max, m_freq_min};

    ASSERT_EQ(m_mapped_freqs.size(), m_region_names.size());

    std::stringstream ss;
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        ss << m_region_names[x] << ":" << m_mapped_freqs[x] << ",";
    }

    setenv("GEOPM_SIMPLE_FREQ_MIN", std::to_string(m_freq_min).c_str(), 1);
    setenv("GEOPM_SIMPLE_FREQ_MAX", std::to_string(m_freq_max).c_str(), 1);
    setenv("GEOPM_SIMPLE_FREQ_RID_MAP", ss.str().c_str(), 1);

    m_mockregion = new MockRegion();
    m_mockpolicy = new MockPolicy();
    m_fact = new geopm::DeciderFactory();
    m_decider = m_fact->decider("simple_freq");
}

void SimpleFreqDeciderTest::TearDown()
{
    if (m_decider) {
        delete m_decider;
    }
    if (m_fact) {
        delete m_fact;
    }
    if (m_mockregion) {
        delete m_mockregion;
    }
    if (m_mockpolicy) {
        delete m_mockpolicy;
    }
}

TEST_F(SimpleFreqDeciderTest, map)
{
    Sequence s1;
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        double expected_freq = m_mapped_freqs[x];
        EXPECT_CALL(*m_mockpolicy, ctl_cpu_freq(_))
            .InSequence(s1)
            .WillOnce(Invoke([expected_freq] (std::vector<double> freq)
                    {
                        for (auto &cpu_freq : freq) {
                            EXPECT_EQ(expected_freq, cpu_freq);
                        }
                    }));
    }

    Sequence s2;
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        EXPECT_CALL(*m_mockregion, identifier())
            .InSequence(s2)
            // one for super, once for our decider
            .WillOnce(Return(geopm_crc32_str(0, m_region_names[x].c_str())))
            .WillOnce(Return(geopm_crc32_str(0, m_region_names[x].c_str())));
    }

    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        m_decider->update_policy(*m_mockregion, *m_mockpolicy);
    }
}

TEST_F(SimpleFreqDeciderTest, decider_is_supported)
{
    EXPECT_TRUE(m_decider->decider_supported("simple_freq"));
    EXPECT_FALSE(m_decider->decider_supported("bad_string"));
}

TEST_F(SimpleFreqDeciderTest, name)
{
    EXPECT_TRUE(std::string("simple_freq") == m_decider->name());
}

TEST_F(SimpleFreqDeciderTest, clone)
{
    geopm::IDecider *cloned = m_decider->clone();
    EXPECT_TRUE(std::string("simple_freq") == cloned->name());
    delete cloned;
}

TEST_F(SimpleFreqDeciderTest, hint)
{
    Sequence s1;
    for (auto &expected_freq : m_expected_freqs) {
        EXPECT_CALL(*m_mockpolicy, ctl_cpu_freq(_))
            .InSequence(s1)
            .WillOnce(Invoke([expected_freq] (std::vector<double> freq)
                {
                    for (auto &cpu_freq : freq) {
                        EXPECT_EQ(expected_freq, cpu_freq);
                    }
                }));
    }

    Sequence s2;
    for (size_t x = 0; x < m_hints.size(); x++) {
        EXPECT_CALL(*m_mockregion, hint())
            .InSequence(s2)
            .WillOnce(testing::Return(m_hints[x]));
    }

    for (size_t x = 0; x < m_hints.size(); x++) {
        m_decider->update_policy(*m_mockregion, *m_mockpolicy);
    }
}

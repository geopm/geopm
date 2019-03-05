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

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_internal.h"
#include "geopm_hash.h"

#include "Agent.hpp"
#include "FrequencyMapAgent.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "Agg.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "PlatformTopo.hpp"
#include "geopm.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Sequence;
using ::testing::Return;
using ::testing::AtLeast;
using geopm::FrequencyMapAgent;
using geopm::PlatformTopo;
using geopm::IPlatformIO;

class FrequencyMapAgentTest : public :: testing :: Test
{
    protected:
        enum mock_pio_idx_e {
            REGION_HASH_IDX,
            REGION_HINT_IDX,
            FREQ_CONTROL_IDX,
        };

        void SetUp();
        void TearDown();
        static const int M_NUM_CPU = 1;
        static const size_t M_NUM_REGIONS = 5;
        std::vector<double> m_expected_freqs;
        std::unique_ptr<FrequencyMapAgent> m_agent;
        std::vector<std::string> m_region_names;
        std::vector<uint64_t> m_region_hash;
        std::vector<uint64_t> m_region_hint;
        std::vector<double> m_mapped_freqs;
        std::vector<double> m_default_policy;
        double m_freq_min;
        double m_freq_max;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
};

void FrequencyMapAgentTest::SetUp()
{
    m_platform_io = geopm::make_unique<MockPlatformIO>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();
    ON_CALL(*m_platform_topo, num_domain(PlatformTopo::M_DOMAIN_CPU))
        .WillByDefault(Return(M_NUM_CPU));
    ON_CALL(*m_platform_io, signal_domain_type(_))
        .WillByDefault(Return(PlatformTopo::M_DOMAIN_BOARD));
    ON_CALL(*m_platform_io, control_domain_type(_))
        .WillByDefault(Return(PlatformTopo::M_DOMAIN_CPU));
    ON_CALL(*m_platform_io, read_signal(std::string("CPUINFO::FREQ_MIN"), _, _))
        .WillByDefault(Return(1.0e9));
    ON_CALL(*m_platform_io, read_signal(std::string("CPUINFO::FREQ_STICKER"), _, _))
        .WillByDefault(Return(1.3e9));
    ON_CALL(*m_platform_io, read_signal(std::string("CPUINFO::FREQ_MAX"), _, _))
        .WillByDefault(Return(2.2e9));
    ON_CALL(*m_platform_io, read_signal(std::string("CPUINFO::FREQ_STEP"), _, _))
        .WillByDefault(Return(100e6));
    ON_CALL(*m_platform_io, push_signal("REGION_HASH", _, _))
        .WillByDefault(Return(REGION_HASH_IDX));
    ON_CALL(*m_platform_io, push_signal("REGION_HINT", _, _))
        .WillByDefault(Return(REGION_HINT_IDX));
    ON_CALL(*m_platform_io, push_control("FREQUENCY", _, _))
        .WillByDefault(Return(FREQ_CONTROL_IDX));
    ON_CALL(*m_platform_io, agg_function(_))
        .WillByDefault(Return(geopm::Agg::max));
    EXPECT_CALL(*m_platform_io, agg_function(_))
        .WillRepeatedly(Return(geopm::Agg::max));

    // calls in constructor
    EXPECT_CALL(*m_platform_topo, num_domain(_)).Times(AtLeast(1));
    EXPECT_CALL(*m_platform_io, signal_domain_type(_)).Times(AtLeast(1));
    EXPECT_CALL(*m_platform_io, control_domain_type(_)).Times(AtLeast(1));
    EXPECT_CALL(*m_platform_io, read_signal(_, _, _)).Times(AtLeast(1));
    EXPECT_CALL(*m_platform_io, push_signal(_, _, _)).Times(AtLeast(1));
    EXPECT_CALL(*m_platform_io, push_control(_, _, _)).Times(M_NUM_CPU);

    m_freq_min = 1800000000.0;
    m_freq_max = 2200000000.0;
    m_region_names = {"mapped_region0", "mapped_region1", "mapped_region2", "mapped_region3", "mapped_region4"};
    m_region_hash = {0xeffa9a8d, 0x4abb08f3, 0xa095c880, 0x5d45afe, 0x71243e97};
    m_mapped_freqs = {m_freq_max, 2100000000.0, 2000000000.0, 1900000000.0, m_freq_min};
    m_default_policy = {m_freq_min, m_freq_max};

    // order of hints should alternate between min and max expected frequency
    m_region_hint = {GEOPM_REGION_HINT_COMPUTE, GEOPM_REGION_HINT_MEMORY,
                     GEOPM_REGION_HINT_SERIAL, GEOPM_REGION_HINT_NETWORK,
                     GEOPM_REGION_HINT_PARALLEL, GEOPM_REGION_HINT_IO,
                     GEOPM_REGION_HINT_IGNORE, GEOPM_REGION_HINT_NETWORK,
                     GEOPM_REGION_HINT_UNKNOWN};
    m_expected_freqs = {m_freq_min, m_freq_max, m_freq_min, m_freq_max, m_freq_min};

    ASSERT_EQ(m_mapped_freqs.size(), m_region_names.size());
    ASSERT_EQ(m_mapped_freqs.size(), m_region_hash.size());
    ASSERT_EQ(m_mapped_freqs.size(), m_expected_freqs.size());

    std::stringstream ss;
    ss << "{";
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        ss << "\"" << m_region_names[x] << "\": " << m_mapped_freqs[x];
        if (x != M_NUM_REGIONS-1) {
            ss << ", ";
        }
    }
    ss << "}";

    setenv("GEOPM_FREQUENCY_MAP", ss.str().c_str(), 1);

    m_agent = geopm::make_unique<FrequencyMapAgent>(*m_platform_io, *m_platform_topo);
    m_agent->init(0, {}, false);
}

void FrequencyMapAgentTest::TearDown()
{
    unsetenv("GEOPM_FREQUENCY_MAP");
}

TEST_F(FrequencyMapAgentTest, map)
{
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        EXPECT_CALL(*m_platform_io, sample(REGION_HASH_IDX))
            .WillOnce(Return(m_region_hash[x]));
        EXPECT_CALL(*m_platform_io, sample(REGION_HINT_IDX))
            .WillOnce(Return(m_region_hint[x]));
        std::vector<double> tmp;
        m_agent->sample_platform(tmp);
        EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, m_mapped_freqs[x])).Times(M_NUM_CPU);
        m_agent->adjust_platform(m_default_policy);
    }
}

TEST_F(FrequencyMapAgentTest, name)
{
    EXPECT_EQ("frequency_map", m_agent->plugin_name());
    EXPECT_NE("bad_string", m_agent->plugin_name());
}

TEST_F(FrequencyMapAgentTest, hint)
{
    for (size_t x = 0; x < m_region_hint.size(); x++) {
        EXPECT_CALL(*m_platform_io, sample(REGION_HASH_IDX))
            .WillOnce(Return(0x1234 + x));
        EXPECT_CALL(*m_platform_io, sample(REGION_HINT_IDX))
            .WillOnce(Return(m_region_hint[x]));
        double expected_freq = NAN;
        switch(m_region_hint[x]) {
            // Hints for low CPU frequency
            case GEOPM_REGION_HINT_MEMORY:
            case GEOPM_REGION_HINT_NETWORK:
            case GEOPM_REGION_HINT_IO:
                expected_freq = m_freq_min;
                break;
            // Hints for maximum CPU frequency
            case GEOPM_REGION_HINT_COMPUTE:
            case GEOPM_REGION_HINT_SERIAL:
            case GEOPM_REGION_HINT_PARALLEL:
                expected_freq = m_freq_max;
                break;
            // Hint Inconclusive
            //case GEOPM_REGION_HINT_UNKNOWN:
            //case GEOPM_REGION_HINT_IGNORE:
            default:
                expected_freq = m_freq_max;
                break;
        }
        EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, expected_freq)).Times(M_NUM_CPU);
        std::vector<double> tmp;
        m_agent->sample_platform(tmp);
        m_agent->adjust_platform(m_default_policy);
    }
}


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
#include "EnergyEfficientAgent.hpp"
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
using geopm::EnergyEfficientAgent;
using geopm::PlatformTopo;
using geopm::IPlatformIO;

class EnergyEfficientAgentTest : public :: testing :: Test
{
    protected:
        enum mock_pio_idx_e {
            REGION_ID_IDX,
            RUNTIME_IDX,
            FREQ_CONTROL_IDX,
            ENERGY_PKG_IDX,
            FREQ_SIGNAL_IDX,
        };

        void SetUp();
        void TearDown();
        static const size_t M_NUM_REGIONS = 5;
        std::vector<size_t> m_hints;
        std::vector<double> m_expected_freqs;
        std::unique_ptr<EnergyEfficientAgent> m_agent;
        std::vector<std::string> m_region_names;
        std::vector<uint64_t> m_region_hash;
        std::vector<double> m_mapped_freqs;
        std::vector<double> m_sample;
        std::vector<double> m_default_policy;
        double m_freq_min;
        double m_freq_max;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
        const int M_NUM_CPU = 4;
};

void EnergyEfficientAgentTest::SetUp()
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
    ON_CALL(*m_platform_io, push_signal("REGION_ID#", _, _))
        .WillByDefault(Return(REGION_ID_IDX));
    ON_CALL(*m_platform_io, push_signal("REGION_RUNTIME", _, _))
        .WillByDefault(Return(RUNTIME_IDX));
    ON_CALL(*m_platform_io, push_signal("ENERGY_PACKAGE", _, _))
        .WillByDefault(Return(ENERGY_PKG_IDX));
    ON_CALL(*m_platform_io, push_control("FREQUENCY", _, _))
        .WillByDefault(Return(FREQ_CONTROL_IDX));
    ON_CALL(*m_platform_io, push_signal("FREQUENCY", _, _))
        .WillByDefault(Return(FREQ_SIGNAL_IDX));
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
    m_hints = {GEOPM_REGION_HINT_COMPUTE, GEOPM_REGION_HINT_MEMORY,
               GEOPM_REGION_HINT_SERIAL, GEOPM_REGION_HINT_NETWORK,
               GEOPM_REGION_HINT_PARALLEL, GEOPM_REGION_HINT_IO,
               GEOPM_REGION_HINT_IGNORE, GEOPM_REGION_HINT_NETWORK,
               GEOPM_REGION_HINT_UNKNOWN};
    m_expected_freqs = {m_freq_min, m_freq_max, m_freq_min, m_freq_max, m_freq_min};
    m_sample.resize(2);

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

    setenv("GEOPM_EFFICIENT_FREQ_RID_MAP", ss.str().c_str(), 1);

    m_agent = geopm::make_unique<EnergyEfficientAgent>(*m_platform_io, *m_platform_topo);
}

void EnergyEfficientAgentTest::TearDown()
{
    unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");
}

TEST_F(EnergyEfficientAgentTest, map)
{
    EXPECT_CALL(*m_platform_io, sample(ENERGY_PKG_IDX))
        .Times(M_NUM_REGIONS)
        .WillRepeatedly(Return(8888));

    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        EXPECT_CALL(*m_platform_io, sample(REGION_ID_IDX))
            .WillOnce(Return(geopm_field_to_signal(m_region_hash[x])));
        EXPECT_CALL(*m_platform_io, sample(FREQ_SIGNAL_IDX))
            .WillOnce(Return(1.2e9));
        m_agent->sample_platform(m_sample);
        EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, m_mapped_freqs[x])).Times(M_NUM_CPU);
        m_agent->adjust_platform(m_default_policy);
    }
}

TEST_F(EnergyEfficientAgentTest, name)
{
    EXPECT_EQ("energy_efficient", m_agent->plugin_name());
    EXPECT_NE("bad_string", m_agent->plugin_name());
}

TEST_F(EnergyEfficientAgentTest, hint)
{
    EXPECT_CALL(*m_platform_io, sample(ENERGY_PKG_IDX))
        .Times(m_hints.size())
        .WillRepeatedly(Return(8888));

    for (size_t x = 0; x < m_hints.size(); x++) {
        EXPECT_CALL(*m_platform_io, sample(REGION_ID_IDX))
            .WillOnce(Return(geopm_field_to_signal(
                geopm_region_id_set_hint(m_hints[x], 0x1234 + x))));
        EXPECT_CALL(*m_platform_io, sample(FREQ_SIGNAL_IDX))
            .WillOnce(Return(1.2e9));
        double expected_freq = NAN;
        switch(m_hints[x]) {
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
        m_agent->sample_platform(m_sample);
        m_agent->adjust_platform(m_default_policy);
    }
}

TEST_F(EnergyEfficientAgentTest, online_mode)
{
    int err = unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");
    EXPECT_EQ(0, err);
    EXPECT_EQ(NULL, getenv("GEOPM_EFFICIENT_FREQ_RID_MAP"));
    setenv("GEOPM_EFFICIENT_FREQ_ONLINE", "yes", 1);
    double freq_min = 1e9;
    double freq_max = 2e9;
    m_default_policy = {freq_min, freq_max};

    for (int x = 0; x < 4; ++x) {
        // calls in constructor fromm SetUp and this test
        EXPECT_CALL(*m_platform_io, signal_domain_type(_)).Times(1);
        EXPECT_CALL(*m_platform_io, control_domain_type(_)).Times(1);
        EXPECT_CALL(*m_platform_io, read_signal(_, _, _)).Times(2);
        EXPECT_CALL(*m_platform_io, push_signal("REGION_ID#", _, _)).Times(1);
        EXPECT_CALL(*m_platform_io, push_control("FREQUENCY", _, _)).Times(M_NUM_CPU);
        EXPECT_CALL(*m_platform_io, push_signal("REGION_RUNTIME", _, _)).Times(1);
        EXPECT_CALL(*m_platform_io, push_signal("ENERGY_PACKAGE", _, _)).Times(2);

        // reset agent with new settings
        m_agent = geopm::make_unique<EnergyEfficientAgent>(*m_platform_io, *m_platform_topo);

        {
            // within EfficientFreqRegion
            EXPECT_CALL(*m_platform_io, sample(REGION_ID_IDX))
                .WillOnce(Return(geopm_region_id_set_hint(m_hints[x], m_region_hash[x])));
            EXPECT_CALL(*m_platform_io, sample(FREQ_SIGNAL_IDX))
                .WillOnce(Return(1.2e9));
            EXPECT_CALL(*m_platform_io, sample(ENERGY_PKG_IDX)).Times(2);
            EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, _)).Times(M_NUM_CPU);
            m_agent->sample_platform(m_sample);
            m_agent->adjust_platform(m_default_policy);
        }
    }

    unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");
}

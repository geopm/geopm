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

#include "contrib/json11/json11.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_agent.h"
#include "geopm_internal.h"
#include "geopm_hash.h"

#include "Agent.hpp"
#include "FrequencyMapAgent.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "Agg.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockFrequencyGovernor.hpp"
#include "PlatformTopo.hpp"
#include "geopm.h"
#include "geopm_test.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Sequence;
using ::testing::Return;
using ::testing::AtLeast;
using geopm::FrequencyMapAgent;
using geopm::PlatformTopo;
using json11::Json;

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
        double m_freq_step;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
        std::shared_ptr<MockFrequencyGovernor> m_governor;
};

void FrequencyMapAgentTest::SetUp()
{
    m_platform_io = geopm::make_unique<MockPlatformIO>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();
    m_governor = std::make_shared<MockFrequencyGovernor>();
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(1));
    ON_CALL(*m_platform_io, push_signal("REGION_HASH", _, _))
        .WillByDefault(Return(REGION_HASH_IDX));
    ON_CALL(*m_platform_io, push_signal("REGION_HINT", _, _))
        .WillByDefault(Return(REGION_HINT_IDX));
    ON_CALL(*m_platform_io, agg_function(_))
        .WillByDefault(Return(geopm::Agg::max));

    m_freq_min = 1800000000.0;
    m_freq_max = 2200000000.0;
    m_freq_step = 100000000.0;
    ON_CALL(*m_governor, frequency_domain_type())
        .WillByDefault(Return(GEOPM_DOMAIN_BOARD));
    ON_CALL(*m_governor, get_frequency_min())
        .WillByDefault(Return(m_freq_min));
    ON_CALL(*m_governor, get_frequency_max())
        .WillByDefault(Return(m_freq_max));
    ON_CALL(*m_governor, get_frequency_step())
        .WillByDefault(Return(m_freq_step));

    // calls in constructor
    EXPECT_CALL(*m_platform_topo, num_domain(_)).Times(AtLeast(1));
    EXPECT_CALL(*m_platform_io, push_signal(_, _, _)).Times(AtLeast(1));

    m_region_names = {"mapped_region0", "mapped_region1", "mapped_region2", "mapped_region3", "mapped_region4"};
    m_region_hash = {0xeffa9a8d, 0x4abb08f3, 0xa095c880, 0x5d45afe, 0x71243e97};
    m_mapped_freqs = {m_freq_max, 2100000000.0, 2000000000.0, 1900000000.0, m_freq_min};
    m_default_policy = {m_freq_min, m_freq_max};

    for (size_t i = 0; i < M_NUM_REGIONS; ++i) {
        m_default_policy.push_back(static_cast<double>(m_region_hash[i]));
        m_default_policy.push_back(m_mapped_freqs[i]);
    }

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

    std::map<uint64_t, double> frequency_map;
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        frequency_map[m_region_hash[x]] = m_mapped_freqs[x];
    }

    m_agent = geopm::make_unique<FrequencyMapAgent>(
        *m_platform_io, *m_platform_topo, m_governor, std::map<uint64_t, double>{});
    // todo: this test assumes board domain is used for control
    EXPECT_CALL(*m_governor, init_platform_io());
    EXPECT_CALL(*m_governor, frequency_domain_type());
    m_agent->init(0, {}, false);
}

void FrequencyMapAgentTest::TearDown()
{

}

TEST_F(FrequencyMapAgentTest, map)
{
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        EXPECT_CALL(*m_platform_io, sample(REGION_HASH_IDX))
            .WillOnce(Return(m_region_hash[x]));
        EXPECT_CALL(*m_platform_io, sample(REGION_HINT_IDX))
            .WillOnce(Return(m_region_hint[x]));
        std::vector<double> adjust_vals = {m_mapped_freqs[x]};
        EXPECT_CALL(*m_governor, set_frequency_bounds(m_freq_min, m_freq_max));
        EXPECT_CALL(*m_governor, adjust_platform(adjust_vals));
        EXPECT_CALL(*m_governor, do_write_batch())
            .WillOnce(Return(true));
        std::vector<double> tmp;
        m_agent->sample_platform(tmp);
        m_agent->adjust_platform(m_default_policy);
        EXPECT_TRUE(m_agent->do_write_batch());
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
        std::vector<double> tmp;
        m_agent->sample_platform(tmp);
        EXPECT_CALL(*m_governor, set_frequency_bounds(m_freq_min, m_freq_max));
        std::vector<double> adjust_vals = {expected_freq};
        EXPECT_CALL(*m_governor, adjust_platform(adjust_vals));
        EXPECT_CALL(*m_governor, do_write_batch())
            .WillOnce(Return(true));
        m_agent->adjust_platform(m_default_policy);
        EXPECT_TRUE(m_agent->do_write_batch());
    }
}

TEST_F(FrequencyMapAgentTest, enforce_policy)
{
    const double limit = 1e9;
    const std::vector<double> policy{0, limit};
    const std::vector<double> bad_policy(123, 100);

    EXPECT_CALL(*m_platform_io, write_control("FREQUENCY", GEOPM_DOMAIN_BOARD, 0, limit));

    m_agent->enforce_policy(policy);

    EXPECT_THROW(m_agent->enforce_policy(bad_policy), geopm::Exception);
}

static Json get_freq_map_json_from_policy(const std::vector<double> &policy)
{
    std::array<char, 4096> policy_buf;
    EXPECT_EQ(0, geopm_agent_policy_json_partial("frequency_map", policy.size(),
                                                 policy.data(), policy_buf.size(),
                                                 policy_buf.data()));
    std::string parse_error;
    return Json::parse(policy_buf.data(), parse_error);
}

TEST_F(FrequencyMapAgentTest, policy_to_json)
{
    EXPECT_EQ(Json(Json::object{ { "FREQ_MIN", 0 }, { "FREQ_MAX", 3e9 } }),
              get_freq_map_json_from_policy({ 0, 3e9 }));
    EXPECT_EQ(Json(Json::object{ { "FREQ_MIN", 0 }, { "FREQ_MAX", 1e40 } }),
              get_freq_map_json_from_policy({ 0, 1e40 }));
    EXPECT_EQ(Json(Json::object{ { "FREQ_MIN", 0 }, { "FREQ_MAX", 1e-40 } }),
              get_freq_map_json_from_policy({ 0, 1e-40 }));
}

TEST_F(FrequencyMapAgentTest, validate_policy)
{
    using ::testing::Pointwise;
    using ::testing::NanSensitiveDoubleEq;

    std::vector<double> policy{400, 500, 123, 456, NAN, NAN};
    m_agent->validate_policy(policy);
    EXPECT_THAT(policy, Pointwise(NanSensitiveDoubleEq(), std::vector<double>{400, 500, 123, 456, NAN, NAN}));

    policy = {400, 500, 123, 456, 123, 450};
    EXPECT_THROW(m_agent->validate_policy(policy), geopm::Exception)
        << "Region with multiple mapped frequencies";

    policy = {400, 500, 123, NAN};
    m_agent->validate_policy(policy);
    EXPECT_EQ(400, policy[0]);
    EXPECT_EQ(500, policy[1]);
    EXPECT_EQ(123, policy[2]);
    EXPECT_TRUE(std::isnan(policy[3]));

    policy = {400, 500, NAN, NAN, 0x123, 2e9};
    m_agent->validate_policy(policy);
    EXPECT_THAT(policy, Pointwise(NanSensitiveDoubleEq(), std::vector<double>{400, 500, NAN, NAN, 0x123, 2e9}))
        << "Gap in policy";

    policy = {400, 500, NAN, 456};
    EXPECT_THROW(m_agent->validate_policy(policy), geopm::Exception)
        << "Frequency without region";
}

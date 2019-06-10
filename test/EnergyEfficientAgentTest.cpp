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

#include <cmath>

#include <vector>
#include <memory>
#include <map>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "EnergyEfficientAgent.hpp"
#include "geopm.h"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockFrequencyGovernor.hpp"
#include "MockEnergyEfficientRegion.hpp"
#include "Helper.hpp"
#include "geopm_test.hpp"
#include "config.h"

using geopm::EnergyEfficientAgent;
using geopm::EnergyEfficientRegion;

using testing::Return;
using testing::_;
using testing::DoAll;
using testing::SetArgReferee;
using testing::AtLeast;

class EnergyEfficientAgentTest : public :: testing :: Test
{
    protected:
        void SetUp();
        MockPlatformIO m_platio;
        MockPlatformTopo m_topo;
        std::shared_ptr<MockFrequencyGovernor> m_gov;
        std::unique_ptr<EnergyEfficientAgent> m_agent0;
        std::unique_ptr<EnergyEfficientAgent> m_agent1;
        static constexpr int M_NUM_CHILDREN = 3;
        static constexpr int M_FREQ_DOMAIN = GEOPM_DOMAIN_CORE;
        static constexpr int M_NUM_FREQ_DOMAIN = 4;
        static constexpr double M_PERF_MARGIN = 0.10;
        // offsets for PlatformIO signal indices
        const int HASH_SIG = 1000;
        const int HINT_SIG = 2000;
        const int RUNTIME_SIG = 3000;
        const int COUNT_SIG = 4000;
        std::map<uint64_t, std::shared_ptr<MockEnergyEfficientRegion> > m_region_map;
};

void EnergyEfficientAgentTest::SetUp()
{
    m_gov = std::make_shared<MockFrequencyGovernor>();

    ON_CALL(*m_gov, frequency_domain_type())
        .WillByDefault(Return(M_FREQ_DOMAIN));
    ON_CALL(m_topo, num_domain(M_FREQ_DOMAIN))
        .WillByDefault(Return(M_NUM_FREQ_DOMAIN));

    std::vector<int> fan_in {M_NUM_CHILDREN};
    m_region_map[0x12] = std::make_shared<MockEnergyEfficientRegion>();
    m_region_map[0x34] = std::make_shared<MockEnergyEfficientRegion>();
    m_region_map[0x56] = std::make_shared<MockEnergyEfficientRegion>();
    // expectations for constructor
    EXPECT_CALL(m_topo, num_domain(M_FREQ_DOMAIN)).Times(2);
    EXPECT_CALL(*m_gov, frequency_domain_type()).Times(2);
    for (int idx = 0; idx < M_NUM_FREQ_DOMAIN; ++idx) {
        EXPECT_CALL(m_platio, push_signal("REGION_HASH", M_FREQ_DOMAIN, idx))
            .WillOnce(Return(HASH_SIG + idx));
        EXPECT_CALL(m_platio, push_signal("REGION_HINT", M_FREQ_DOMAIN, idx))
            .WillOnce(Return(HINT_SIG + idx));
        EXPECT_CALL(m_platio, push_signal("REGION_RUNTIME", M_FREQ_DOMAIN, idx))
            .WillOnce(Return(RUNTIME_SIG + idx));
        EXPECT_CALL(m_platio, push_signal("REGION_COUNT", M_FREQ_DOMAIN, idx))
            .WillOnce(Return(COUNT_SIG + idx));
    }
    std::map<uint64_t, std::shared_ptr<EnergyEfficientRegion> > region_map;
    for (auto &kv : m_region_map) {
        region_map[kv.first] = kv.second;
    }
    m_agent0 = geopm::make_unique<EnergyEfficientAgent>(m_platio, m_topo, m_gov, region_map);
    m_agent1 = geopm::make_unique<EnergyEfficientAgent>(m_platio, m_topo, m_gov,
                                                        std::map<uint64_t, std::shared_ptr<EnergyEfficientRegion> >());

    // expectations for init
    EXPECT_CALL(*m_gov, init_platform_io()).Times(1);
    m_agent0->init(0, fan_in, false);
    m_agent1->init(1, fan_in, false);
}

TEST_F(EnergyEfficientAgentTest, validate_policy_default)
{
    // set up expectations for NAN policy: system min and max
    double sys_min = 1.0e9;
    double sys_max = 2.0e9;
    EXPECT_CALL(*m_gov, validate_policy(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(sys_min),
                        SetArgReferee<1>(sys_max)));
    std::vector<double> in_policy {NAN, NAN, NAN};
    std::vector<double> expected {sys_min, sys_max, 0.10};
    m_agent0->validate_policy(in_policy);
    EXPECT_EQ(expected, in_policy);
}

TEST_F(EnergyEfficientAgentTest, validate_policy_clamp)
{
    EXPECT_CALL(*m_gov, validate_policy(_, _));
    // validate policy does not do clamping for frequency
    std::vector<double> wide_policy {0.9e9, 2.1e9, 0.5};
    std::vector<double> in_policy = wide_policy;
    m_agent0->validate_policy(in_policy);
    EXPECT_EQ(wide_policy, in_policy);
}

TEST_F(EnergyEfficientAgentTest, validate_policy_perf_margin)
{
    std::vector<double> in_policy {NAN, NAN, -0.2};
    EXPECT_THROW(m_agent0->validate_policy(in_policy), geopm::Exception);
    in_policy = {NAN, NAN, 1.2};
    EXPECT_THROW(m_agent0->validate_policy(in_policy), geopm::Exception);
}

TEST_F(EnergyEfficientAgentTest, split_policy_unchanged)
{
    double in_pol_min = 1.1e9;
    double in_pol_max = 2.1e9;
    std::vector<double> in_policy {in_pol_min, in_pol_max, M_PERF_MARGIN};
    std::vector<double> garbage {5.67, 8.90, 7.8};
    std::vector<std::vector<double> > out_policy {M_NUM_CHILDREN, garbage};

    EXPECT_CALL(*m_gov, set_frequency_bounds(in_pol_min, in_pol_max))
        .WillOnce(Return(false));
    m_agent1->split_policy(in_policy, out_policy);
    EXPECT_FALSE(m_agent1->do_send_policy());
    // out_policy will not be modified
    for (const auto &child_policy : out_policy) {
        EXPECT_EQ(garbage, child_policy);
    }
}

TEST_F(EnergyEfficientAgentTest, split_policy_changed)
{
    double in_pol_min = 1.1e9;
    double in_pol_max = 2.1e9;
    std::vector<double> in_policy {in_pol_min, in_pol_max, M_PERF_MARGIN};
    std::vector<double> garbage {5.67, 8.90, 7.9};
    std::vector<std::vector<double> > out_policy {M_NUM_CHILDREN, garbage};

    EXPECT_CALL(*m_gov, set_frequency_bounds(in_pol_min, in_pol_max))
        .WillOnce(Return(true));
    m_agent1->split_policy(in_policy, out_policy);
    EXPECT_TRUE(m_agent1->do_send_policy());
    for (const auto &child_policy : out_policy) {
        EXPECT_EQ(in_policy, child_policy);
    }
}

TEST_F(EnergyEfficientAgentTest, split_policy_errors)
{
#ifdef GEOPM_DEBUG
    std::vector<double> in_policy {1.2e9, 1.4e9, M_PERF_MARGIN};
    std::vector<std::vector<double> > out_policy {M_NUM_CHILDREN, in_policy};
    std::vector<double> bad_in {4, 4, 4, 4, 4, 4};
    std::vector<std::vector<double> > bad_out1 {8, in_policy};
    std::vector<std::vector<double> > bad_out2 {M_NUM_CHILDREN, bad_in};

    GEOPM_EXPECT_THROW_MESSAGE(m_agent1->split_policy(bad_in, out_policy), GEOPM_ERROR_LOGIC,
                               "in_policy vector not correctly sized");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent1->split_policy(in_policy, bad_out1), GEOPM_ERROR_LOGIC,
                               "out_policy vector not correctly sized");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent1->split_policy(in_policy, bad_out2), GEOPM_ERROR_LOGIC,
                               "child_policy vector not correctly sized");
#endif
}

TEST_F(EnergyEfficientAgentTest, aggregate_sample)
{
    std::vector<double> empty;
    std::vector<double> out_sample;
    std::vector<std::vector<double> > in_sample {M_NUM_CHILDREN, empty};
    m_agent0->aggregate_sample(in_sample, out_sample);
    EXPECT_FALSE(m_agent0->do_send_sample());
    // no samples to aggregate
    EXPECT_EQ(empty, out_sample);
}

TEST_F(EnergyEfficientAgentTest, do_write_batch)
{
    // pass through to FrequencyGovernor
    EXPECT_CALL(*m_gov, do_write_batch())
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_TRUE(m_agent0->do_write_batch());
    EXPECT_FALSE(m_agent0->do_write_batch());
}

TEST_F(EnergyEfficientAgentTest, sample_adjust_platform)
{
    constexpr int NUM_STEPS = 8;
    // some non-zero value; freq() will be injected so actual runtimes don't matter
    double runtime = 4.2;
    double min = 1.0e9;
    double lo1 = 1.2e9;
    double lo2 = 1.3e9;
    double med = 1.5e9;
    double max = 2.0e9;
    std::vector<double> in_policy {min, max, M_PERF_MARGIN};

    uint64_t UM = GEOPM_REGION_HASH_UNMARKED;
    std::map<uint64_t, uint64_t> region_hints = {
        {0x12, GEOPM_REGION_HINT_MEMORY},
        {0x34, GEOPM_REGION_HINT_UNKNOWN},
        {0x56, GEOPM_REGION_HINT_COMPUTE},
        {  UM, GEOPM_REGION_HINT_UNKNOWN}
    };
    // which region will be returned by pio sample
    uint64_t step_region[NUM_STEPS][M_NUM_FREQ_DOMAIN] = {
        {0x12, 0x34, 0x56,   UM},  // make sure all regions are entered once
        {  UM,   UM,   UM, 0x56},
        {0x12, 0x12, 0x12, 0x12},
        {0x12, 0x12, 0x34, 0x34},
        {0x12, 0x34, 0x34, 0x34},
        {0x34, 0x34, 0x34,   UM},
        {0x56, 0x56,   UM, 0x56},
        {0x12,   UM, 0x12,   UM},
    };

    // 0 if no leaving a region, or the region hash if calc_next_freq() should be called
    uint64_t do_exit[NUM_STEPS][M_NUM_FREQ_DOMAIN] = {
        {   0,    0,    0,    0},
        {0x12, 0x34, 0x56,    0},
        {   0,    0,    0, 0x56},
        {   0,    0, 0x12, 0x12},
        {   0, 0x12,    0,    0},
        {0x12,    0,    0, 0x34},
        {0x34, 0x34, 0x34,    0},
        {0x56, 0x56,    0, 0x56},
    };
    // expected frequency setting for each core given the above
    // pattern of regions and the hint.  region 0x12 changes to a
    // lower frequency after the second entry
    double exp_freq[NUM_STEPS][M_NUM_FREQ_DOMAIN] = {
        {max, max, max, max},  // default before first region boundary
        {max, max, max, max},
        {lo2, lo2, lo2, lo2},
        {lo2, lo2, med, med},
        {lo2, med, med, med},
        {med, med, med, max},
        {max, max, max, max},
        {lo1, max, lo1, max}
    };

    std::vector<double> out_sample;
    for (int step = 0; step < NUM_STEPS; ++step) {
        std::vector<double> freqs(M_NUM_FREQ_DOMAIN);
        std::vector<uint64_t> named_region = {0x12, 0x34, 0x56};
        std::map<uint64_t, int> freq_call_count = {
            {0x12, 0},
            {0x34, 0},
            {0x56, 0}
        };
        std::map<uint64_t, int> update_call_count = {
            {0x12, 0},
            {0x34, 0},
            {0x56, 0}
        };

        for (int dom = 0; dom < M_NUM_FREQ_DOMAIN; ++dom) {
            uint64_t current_region = step_region[step][dom];
            uint64_t current_hint = region_hints.at(current_region);
            EXPECT_CALL(m_platio, sample(HASH_SIG + dom))
                .WillOnce(Return(current_region));
            EXPECT_CALL(m_platio, sample(HINT_SIG + dom))
                .WillOnce(Return(current_hint));
            EXPECT_CALL(m_platio, sample(RUNTIME_SIG + dom)).WillOnce(Return(runtime));
            EXPECT_CALL(m_platio, sample(COUNT_SIG + dom)).WillOnce(Return(step));
            ++freq_call_count[current_region];
            if (m_region_map.find(current_region) != m_region_map.end()) {
                ON_CALL(*m_region_map.at(current_region), freq())
                    .WillByDefault(Return(exp_freq[step][dom]));
                freqs[dom] = exp_freq[step][dom];
            }
            if (do_exit[step][dom] != 0) {
                ++update_call_count[do_exit[step][dom]];
            }
        }
        // check that EnergyEfficientRegion::freq() was called for the regions in this step
        for (const auto &region : named_region) {
            if (m_region_map.find(region) != m_region_map.end() &&
                region_hints.at(region) != GEOPM_REGION_HINT_COMPUTE) {
                EXPECT_CALL(*m_region_map.at(region), sample(-runtime))
                    .Times(update_call_count.at(region));
                EXPECT_CALL(*m_region_map.at(region), calc_next_freq())
                    .Times(update_call_count.at(region));
            }
            // todo: number of times freq() is called depends on entry
            EXPECT_CALL(*m_region_map.at(region), freq())
                .Times(AtLeast(0));
        }

        EXPECT_CALL(*m_gov, get_frequency_min())
            .WillOnce(Return(min));
        EXPECT_CALL(*m_gov, get_frequency_max())
            .WillOnce(Return(max));
        EXPECT_CALL(*m_gov, get_frequency_step())
            .WillOnce(Return(100e6));
        m_agent0->sample_platform(out_sample);

        EXPECT_CALL(*m_gov, set_frequency_bounds(min, max));
        EXPECT_CALL(*m_gov, get_frequency_max())
            .WillOnce(Return(max));
        EXPECT_CALL(*m_gov, adjust_platform(freqs));
        m_agent0->adjust_platform(in_policy);
    }

    // no extensions to header
    std::vector<std::pair<std::string, std::string> > exp_header;
    EXPECT_EQ(exp_header, m_agent0->report_header());

    // final freq map
    std::vector<std::pair<std::string, std::string> > header = m_agent0->report_host();
    ASSERT_NE(0u, header.size());
    EXPECT_EQ("Final online freq map", header[0].first);

    // frequency for non-compute regions
    auto region_data = m_agent0->report_region();
    EXPECT_EQ(std::to_string(lo1), region_data.at(0x12)[0].second);
    EXPECT_EQ(std::to_string(med), region_data.at(0x34)[0].second);

    // no extensions to trace
    std::vector<std::string> empty;
    EXPECT_EQ(empty, m_agent0->trace_names());
    std::vector<double> vals;
    std::vector<double> result;
    m_agent0->trace_values(result);
    EXPECT_EQ(vals, result);
}

TEST_F(EnergyEfficientAgentTest, static_methods)
{
    EXPECT_EQ("energy_efficient", EnergyEfficientAgent::plugin_name());
    std::vector<std::string> pol_names {"FREQ_MIN", "FREQ_MAX", "PERF_MARGIN"};
    std::vector<std::string> sam_names {};
    EXPECT_EQ(pol_names, EnergyEfficientAgent::policy_names());
    EXPECT_EQ(sam_names, EnergyEfficientAgent::sample_names());
}

TEST_F(EnergyEfficientAgentTest, enforce_policy)
{
    const double limit = 1e9;
    const std::vector<double> policy{0, limit, 0.15};
    const std::vector<double> bad_policy{100, 200, 300, 400};

    EXPECT_CALL(m_platio, write_control("FREQUENCY", GEOPM_DOMAIN_BOARD, 0, limit));

    m_agent0->enforce_policy(policy);

    EXPECT_THROW(m_agent0->enforce_policy(bad_policy), geopm::Exception);
}

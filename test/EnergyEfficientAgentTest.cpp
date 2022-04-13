/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cmath>

#include <vector>
#include <memory>
#include <map>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "EnergyEfficientAgent.hpp"
#include "geopm_prof.h"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockFrequencyGovernor.hpp"
#include "MockEnergyEfficientRegion.hpp"
#include "geopm/Helper.hpp"
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
        // platform values
        double M_SYS_MIN = 1.0e9;
        double M_SYS_MAX = 2.0e9;
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
    EXPECT_CALL(*m_gov, get_frequency_max())
        .WillOnce(Return(M_SYS_MAX));
    m_agent0->init(0, fan_in, false);
    m_agent1->init(1, fan_in, false);
}

TEST_F(EnergyEfficientAgentTest, validate_policy_default)
{
    // set up expectations for NAN policy: system min and max
    EXPECT_CALL(*m_gov, validate_policy(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(M_SYS_MIN),
                        SetArgReferee<1>(M_SYS_MAX)));
    std::vector<double> in_policy {NAN, NAN, NAN, NAN};
    std::vector<double> expected {M_SYS_MIN, M_SYS_MAX, 0.10, M_SYS_MAX};
    ASSERT_EQ(in_policy.size(), m_agent0->policy_names().size());
    EXPECT_CALL(m_platio, read_signal("CPU_FREQUENCY_MAX", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(M_SYS_MAX));
    m_agent0->validate_policy(in_policy);
    EXPECT_EQ(expected, in_policy);
}

TEST_F(EnergyEfficientAgentTest, validate_policy_clamp)
{
    EXPECT_CALL(*m_gov, validate_policy(_, _));
    // validate policy does not do clamping for frequency
    std::vector<double> wide_policy {0.9e9, 2.1e9, 0.5, 2.1e9};
    std::vector<double> in_policy = wide_policy;
    ASSERT_EQ(in_policy.size(), m_agent0->policy_names().size());
    m_agent0->validate_policy(in_policy);
    EXPECT_EQ(wide_policy, in_policy);
}

TEST_F(EnergyEfficientAgentTest, validate_policy_perf_margin)
{
    std::vector<double> in_policy {NAN, NAN, -0.2, NAN};
    EXPECT_THROW(m_agent0->validate_policy(in_policy), geopm::Exception);
    in_policy = {NAN, NAN, 1.2};
    EXPECT_THROW(m_agent0->validate_policy(in_policy), geopm::Exception);
}

TEST_F(EnergyEfficientAgentTest, split_policy_unchanged)
{
    double in_pol_min = 1.1e9;
    double in_pol_max = 2.1e9;
    std::vector<double> in_policy {in_pol_min, in_pol_max, M_PERF_MARGIN, 1.5e9};
    std::vector<double> garbage {5.67, 8.90, 7.8, 9.99};
    std::vector<std::vector<double> > out_policy {M_NUM_CHILDREN, garbage};

    EXPECT_CALL(*m_gov, set_frequency_bounds(in_pol_min, in_pol_max))
        .WillOnce(Return(false));
    ASSERT_EQ(in_policy.size(), m_agent1->policy_names().size());
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
    std::vector<double> in_policy {in_pol_min, in_pol_max, M_PERF_MARGIN, 1.5e9};
    std::vector<double> garbage {5.67, 8.90, 7.9, 9.99};
    std::vector<std::vector<double> > out_policy {M_NUM_CHILDREN, garbage};

    EXPECT_CALL(*m_gov, set_frequency_bounds(in_pol_min, in_pol_max))
        .WillOnce(Return(true));
    ASSERT_EQ(in_policy.size(), m_agent1->policy_names().size());
    m_agent1->split_policy(in_policy, out_policy);
    EXPECT_TRUE(m_agent1->do_send_policy());
    for (const auto &child_policy : out_policy) {
        EXPECT_EQ(in_policy, child_policy);
    }
}

TEST_F(EnergyEfficientAgentTest, split_policy_errors)
{
#ifdef GEOPM_DEBUG
    std::vector<double> in_policy {1.2e9, 1.4e9, M_PERF_MARGIN, 1.5e9};
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

TEST_F(EnergyEfficientAgentTest, static_methods)
{
    EXPECT_EQ("energy_efficient", EnergyEfficientAgent::plugin_name());
    std::vector<std::string> pol_names {"FREQ_MIN", "FREQ_MAX", "PERF_MARGIN", "FREQ_FIXED"};
    std::vector<std::string> sam_names {};
    EXPECT_EQ(pol_names, EnergyEfficientAgent::policy_names());
    EXPECT_EQ(sam_names, EnergyEfficientAgent::sample_names());
}

TEST_F(EnergyEfficientAgentTest, enforce_policy)
{
    const double dynamic_limit = 1.1e9;
    const double static_limit = 1e9;
    const std::vector<double> policy{0, dynamic_limit, 0.15, static_limit};
    const std::vector<double> bad_policy{100, 200, 300, 400, 500, 600};

    EXPECT_CALL(m_platio, write_control("CPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD, 0, static_limit));

    ASSERT_EQ(policy.size(), m_agent0->policy_names().size());
    m_agent0->enforce_policy(policy);

    EXPECT_THROW(m_agent0->enforce_policy(bad_policy), geopm::Exception);
}

/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "FrequencyBalancerAgent.hpp"

#include "MockFrequencyGovernor.hpp"
#include "MockFrequencyLimitDetector.hpp"
#include "MockFrequencyTimeBalancer.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockPowerGovernor.hpp"
#include "MockSSTClosGovernor.hpp"
#include "MockWaiter.hpp"
#include "SSTClosGovernor.hpp"
#include "geopm/Helper.hpp"
#include "geopm_hash.h"
#include "geopm_hint.h"
#include "geopm_test.hpp"

#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <memory>
#include <numeric>

using geopm::FrequencyBalancerAgent;
using geopm::FrequencyTimeBalancer;
using geopm::FrequencyLimitDetector;
using geopm::PlatformTopo;
using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::AtLeast;
using ::testing::Expectation;
using ::testing::Pointwise;
using ::testing::DoubleEq;
using ::testing::ElementsAre;
using ::testing::Ge;
using ::testing::Lt;
using ::testing::AllOf;

const int EPOCH_SIGNAL_IDX = 1000;
const int ACNT_SIGNAL_IDX = 2000;
const int MCNT_SIGNAL_IDX = 3000;
const int REGION_SIGNAL_IDX = 4000;
const int HINT_SIGNAL_IDX = 5000;
const int NETWORK_SIGNAL_IDX = 6000;

const double MAX_FREQ = 3e9;
const double STICKER_FREQ = 2e9;

const double MIN_POWER = 50;
const double TDP_POWER = 100;
const double MAX_POWER = 200;

const int CORE_COUNT = 4;
const int PACKAGE_COUNT = 1;

const auto REGION_SIGNALS = AllOf(Ge(REGION_SIGNAL_IDX), Lt(REGION_SIGNAL_IDX + CORE_COUNT));
const auto HINT_SIGNALS = AllOf(Ge(HINT_SIGNAL_IDX), Lt(HINT_SIGNAL_IDX + CORE_COUNT));

class FrequencyBalancerAgentTest : public ::testing::Test
{
    protected:
        void SetUp();

        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        std::shared_ptr<MockPowerGovernor> m_power_governor;
        std::shared_ptr<MockFrequencyGovernor> m_frequency_governor;
        std::shared_ptr<MockSSTClosGovernor> m_sst_clos_governor;
        std::unique_ptr<FrequencyBalancerAgent> m_agent;
        std::shared_ptr<MockWaiter> m_waiter;
        std::shared_ptr<MockFrequencyTimeBalancer> m_frequency_time_balancer;
        std::shared_ptr<MockFrequencyLimitDetector> m_frequency_limit_detector;

        void expect_initial_batch_control_adjustments();
};

void FrequencyBalancerAgentTest::SetUp()
{
    m_waiter = std::make_shared<MockWaiter>();
    m_power_governor = std::make_shared<MockPowerGovernor>();
    m_frequency_governor = std::make_shared<MockFrequencyGovernor>();
    ON_CALL(*m_frequency_governor, frequency_domain_type()).WillByDefault(Return(GEOPM_DOMAIN_CORE));
    m_sst_clos_governor = std::make_shared<MockSSTClosGovernor>();
    ON_CALL(m_platform_io, read_signal("SST::COREPRIORITY_SUPPORT:CAPABILITIES", _, _))
        .WillByDefault(Return(1));
    ON_CALL(m_platform_io, read_signal("SST::TURBOFREQ_SUPPORT:SUPPORTED", _, _))
        .WillByDefault(Return(1));
    ON_CALL(m_platform_io, read_signal("CPU_POWER_MIN_AVAIL", _, _))
        .WillByDefault(Return(MIN_POWER));
    ON_CALL(m_platform_io, read_signal("CPU_POWER_LIMIT_DEFAULT", _, _))
        .WillByDefault(Return(TDP_POWER));
    ON_CALL(m_platform_io, read_signal("CPU_POWER_MAX_AVAIL", _, _))
        .WillByDefault(Return(MAX_POWER));
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", _, _))
        .WillByDefault(Return(MAX_FREQ));
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_STEP", _, _))
        .WillByDefault(Return(1e8));
    ON_CALL(m_platform_io, read_signal("CPU_FREQUENCY_STICKER", _, _))
        .WillByDefault(Return(STICKER_FREQ));
    ON_CALL(m_platform_io, push_signal("EPOCH_COUNT", _, _)).WillByDefault(Return(EPOCH_SIGNAL_IDX));
    ON_CALL(m_platform_io, push_signal("MSR::APERF:ACNT", _, _)).WillByDefault(Return(ACNT_SIGNAL_IDX));
    ON_CALL(m_platform_io, push_signal("MSR::MPERF:MCNT", _, _)).WillByDefault(Return(MCNT_SIGNAL_IDX));
    ON_CALL(m_platform_io, push_signal("REGION_HASH", GEOPM_DOMAIN_CORE, _))
        .WillByDefault(Invoke([] (const std::string&, int, int core) {
            return REGION_SIGNAL_IDX + core;
        }));
    ON_CALL(m_platform_io, push_signal("REGION_HINT", GEOPM_DOMAIN_CORE, _))
        .WillByDefault(Invoke([] (const std::string&, int, int core) {
            return HINT_SIGNAL_IDX + core;
        }));
    ON_CALL(m_platform_io, push_signal("TIME_HINT_NETWORK", _, _)).WillByDefault(Return(NETWORK_SIGNAL_IDX));

    ON_CALL(m_platform_io, sample(EPOCH_SIGNAL_IDX)).WillByDefault(Return(0));
    ON_CALL(m_platform_io, sample(REGION_SIGNALS)).WillByDefault(Return(GEOPM_REGION_HASH_UNMARKED));
    ON_CALL(m_platform_io, sample(HINT_SIGNALS)).WillByDefault(Return(GEOPM_REGION_HINT_UNKNOWN));
    ON_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(PACKAGE_COUNT));
    ON_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(CORE_COUNT));
    ON_CALL(m_platform_topo, domain_nested(GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_PACKAGE, _))
        .WillByDefault(Invoke([] (int, int, int package_idx) -> std::set<int> {
            // Mock the set of cores per package by evenly dividing cores into each
            // package, in order (e.g., 4 cores in 2 packages are {0, 1}, {2, 3})
            std::vector<int> cores_in_package(CORE_COUNT / PACKAGE_COUNT);
            std::iota(cores_in_package.begin(), cores_in_package.end(), CORE_COUNT / PACKAGE_COUNT * package_idx);
            return std::set<int>(cores_in_package.begin(), cores_in_package.end());
        }));
    ON_CALL(*m_sst_clos_governor, clos_domain_type())
        .WillByDefault(Return(GEOPM_DOMAIN_CORE));

    m_frequency_time_balancer = std::make_shared<MockFrequencyTimeBalancer>();
    m_frequency_limit_detector = std::make_shared<MockFrequencyLimitDetector>();

    m_agent = std::make_unique<FrequencyBalancerAgent>(
        m_platform_io, m_platform_topo, m_waiter, m_power_governor,
        m_frequency_governor, m_sst_clos_governor,
        std::vector<std::shared_ptr<FrequencyTimeBalancer> >{m_frequency_time_balancer},
        m_frequency_limit_detector);
}

TEST_F(FrequencyBalancerAgentTest, adjust_new_epoch)
{
    std::vector<double> out_sample;
    const std::vector<double> POLICY{ NAN, NAN, NAN };
    const auto HP = geopm::SSTClosGovernor::HIGH_PRIORITY;
    const auto LP = geopm::SSTClosGovernor::LOW_PRIORITY;

    m_agent->init(0, {}, false);

    // Only the first agent adjustment expects initial batch adjustments
    EXPECT_CALL(*m_frequency_governor, set_frequency_bounds(_, _));
    Expectation initial_freq_adjust = EXPECT_CALL(*m_frequency_governor,
            adjust_platform(Pointwise(DoubleEq(), { 3e9, 3e9, 3e9, 3e9 }))).Times(1);
    Expectation initial_clos_adjust = EXPECT_CALL(*m_sst_clos_governor,
            adjust_platform(ElementsAre(HP, HP, HP, HP))).Times(1);
    EXPECT_CALL(*m_sst_clos_governor, enable_sst_turbo_prioritization());
    m_agent->sample_platform(out_sample);

    EXPECT_FALSE(m_agent->do_write_batch());
    m_agent->adjust_platform(POLICY);
    // Now there should be batch IO, from initializing our controls
    EXPECT_TRUE(m_agent->do_write_batch());

    // Sample a new epoch and adjust again
    EXPECT_CALL(m_platform_io, sample(EPOCH_SIGNAL_IDX)).WillOnce(Return(10));
    EXPECT_CALL(m_platform_io, sample(ACNT_SIGNAL_IDX)).WillRepeatedly(Return(100));
    EXPECT_CALL(m_platform_io, sample(MCNT_SIGNAL_IDX)).WillRepeatedly(Return(100));
    EXPECT_CALL(m_platform_io, sample(REGION_SIGNALS)).WillRepeatedly(Return(GEOPM_REGION_HASH_UNMARKED));
    EXPECT_CALL(m_platform_io, sample(HINT_SIGNALS)).Times(AtLeast(1));
    EXPECT_CALL(m_platform_io, sample(NETWORK_SIGNAL_IDX)).Times(AtLeast(1))
        .WillRepeatedly(Return(0.1));

    m_agent->sample_platform(out_sample);

    // Make the balancer give a value that needs to be rounded up to a p-state
    // step, plus others that don't need rounding.
    EXPECT_CALL(*m_frequency_time_balancer, balance_frequencies_by_time(_, _, _, _, _))
        .WillOnce(Return(std::vector<double>{2.87e9, 3e9, 2e9, 2.5e9}));
    EXPECT_CALL(*m_frequency_governor,
                adjust_platform(Pointwise(DoubleEq(), {2.9e9, 3e9, 2e9, 2.5e9})))
        .Times(1).After(initial_freq_adjust);

    // Set the test's cutoff frequency. Anything less than or equal to cutoff
    // should get low priority.
    EXPECT_CALL(*m_frequency_limit_detector, get_core_low_priority_frequency(_))
        .WillRepeatedly(Return(2.5e9));
    EXPECT_CALL(*m_sst_clos_governor,
                adjust_platform(ElementsAre( HP, HP, LP, LP )))
        .Times(1).After(initial_clos_adjust);
    m_agent->adjust_platform(POLICY);
}

TEST_F(FrequencyBalancerAgentTest, adjust_frequency_overrides)
{
    std::vector<double> out_sample;
    const std::vector<double> POLICY{ NAN, NAN, NAN };
    const auto HP = geopm::SSTClosGovernor::HIGH_PRIORITY;
    const auto LP = geopm::SSTClosGovernor::LOW_PRIORITY;

    m_agent->init(0, {}, false);

    // Only the first agent adjustment expects initial batch adjustments
    EXPECT_CALL(*m_frequency_governor, set_frequency_bounds(_, _));
    Expectation initial_freq_adjust = EXPECT_CALL(*m_frequency_governor,
            adjust_platform(Pointwise(DoubleEq(), { 3e9, 3e9, 3e9, 3e9 }))).Times(1);
    Expectation initial_clos_adjust = EXPECT_CALL(*m_sst_clos_governor,
            adjust_platform(ElementsAre(HP, HP, HP, HP))).Times(1);
    EXPECT_CALL(*m_sst_clos_governor, enable_sst_turbo_prioritization());
    m_agent->sample_platform(out_sample);
    m_agent->adjust_platform(POLICY);

    // Act like we're still in epoch 0
    EXPECT_CALL(m_platform_io, sample(EPOCH_SIGNAL_IDX))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(m_platform_io, sample(ACNT_SIGNAL_IDX)).WillRepeatedly(Return(100));
    EXPECT_CALL(m_platform_io, sample(MCNT_SIGNAL_IDX)).WillRepeatedly(Return(100));
    EXPECT_CALL(m_platform_io, sample(REGION_SIGNALS))
        .WillRepeatedly(Return(GEOPM_REGION_HASH_UNMARKED));
    // Act like core 0 not in the app. Should throttle.
    EXPECT_CALL(m_platform_io, sample(REGION_SIGNAL_IDX + 0))
        .WillRepeatedly(Return(NAN));
    EXPECT_CALL(m_platform_io, sample(HINT_SIGNALS)).Times(AtLeast(1));
    // Act like core 3 is constantly in a network region. Throttle.
    EXPECT_CALL(m_platform_io, sample(HINT_SIGNAL_IDX + 3))
        .WillRepeatedly(Return(GEOPM_REGION_HINT_NETWORK));

    // Act like we're multiple samples in.
    for (size_t i = 0; i < 5; ++i) {
        m_agent->sample_platform(out_sample);
    }

    // "Previous control" is still "initial control", which is all freq-max.
    // But our non-app core and our always-networking core should be throttled
    // to our cutoff frequency.
    EXPECT_CALL(*m_frequency_limit_detector, get_core_low_priority_frequency(_))
        .WillRepeatedly(Return(2.1e9));
    EXPECT_CALL(*m_frequency_governor,
                adjust_platform(Pointwise(DoubleEq(), {2.1e9, 3e9, 3e9, 2.1e9})))
        .Times(1).After(initial_freq_adjust);
    EXPECT_CALL(*m_sst_clos_governor,
                adjust_platform(ElementsAre( LP, HP, HP, LP )))
        .Times(1).After(initial_clos_adjust);
    m_agent->adjust_platform(POLICY);
}

TEST_F(FrequencyBalancerAgentTest, validate_policy_fills_defaults)
{
    std::vector<double> policy{ NAN, NAN, NAN };
    const double P_STATES_ENABLED = 1;
    const double SST_TF_ENABLED = 1;
    m_agent->validate_policy(policy);
    EXPECT_THAT(policy, ElementsAre(TDP_POWER, P_STATES_ENABLED, SST_TF_ENABLED));
}

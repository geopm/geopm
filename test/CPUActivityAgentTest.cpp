/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_agent.h"
#include "geopm_hash.h"

#include "Agent.hpp"
#include "CPUActivityAgent.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockFrequencyGovernor.hpp"
#include "MockWaiter.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_prof.h"
#include "geopm_test.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Sequence;
using ::testing::Return;
using ::testing::AtLeast;
using ::testing::DoubleNear;
using testing::DoAll;
using testing::SetArgReferee;
using geopm::CPUActivityAgent;
using geopm::PlatformTopo;

class CPUActivityAgentTest : public ::testing::Test
{
    protected:
        enum mock_pio_idx_e {
            QM_CTR_SCALED_RATE_IDX,
            CPU_SCALABILITY_IDX,
            CPU_UNCORE_FREQUENCY_IDX,
            CPU_UNCORE_MIN_CONTROL_IDX,
            CPU_UNCORE_MAX_CONTROL_IDX
        };
        enum policy_idx_e {
            PHI = 0
        };

        void SetUp();
        void setup_default_expect_calls();
        void setup_default_expect_calls_const_config();
        void TearDown();
        static const int M_NUM_CPU;
        static const int M_NUM_CORE;
        static const int M_NUM_BOARD;
        static const int M_NUM_PACKAGE;
        static const size_t M_NUM_UNCORE_MBM_READINGS;
        std::shared_ptr<MockFrequencyGovernor> m_gov;
        std::unique_ptr<CPUActivityAgent> m_agent;
        std::vector<double> m_default_policy;
        size_t m_num_policy;
        double m_cpu_freq_min;
        double m_cpu_freq_max;
        double m_cpu_uncore_freq_min;
        double m_cpu_uncore_freq_max;
        std::vector<double> m_cpu_uncore_freqs;
        std::vector<double> m_mbm_max;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
        std::shared_ptr<MockWaiter> m_waiter;
};

const int CPUActivityAgentTest::M_NUM_CPU = 1;
const int CPUActivityAgentTest::M_NUM_CORE = 2;
const int CPUActivityAgentTest::M_NUM_BOARD = 1;
const int CPUActivityAgentTest::M_NUM_PACKAGE = 1;
const size_t CPUActivityAgentTest::M_NUM_UNCORE_MBM_READINGS = 13;

void CPUActivityAgentTest::SetUp()
{
    m_platform_io = std::make_unique<MockPlatformIO>();
    m_platform_topo = std::make_unique<MockPlatformTopo>();
    m_waiter = std::make_unique<MockWaiter>();

    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(M_NUM_BOARD));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(M_NUM_PACKAGE));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(M_NUM_CORE));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillByDefault(Return(M_NUM_CPU));

    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE)).Times(1);
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CORE)).Times(1);

    // Signals
    ON_CALL(*m_platform_io, push_signal("MSR::QM_CTR_SCALED_RATE", _, _))
        .WillByDefault(Return(QM_CTR_SCALED_RATE_IDX));
    ON_CALL(*m_platform_io, push_signal("MSR::CPU_SCALABILITY_RATIO", _, _))
        .WillByDefault(Return(CPU_SCALABILITY_IDX));
    ON_CALL(*m_platform_io, push_signal("CPU_UNCORE_FREQUENCY_STATUS", _, _))
        .WillByDefault(Return(CPU_UNCORE_FREQUENCY_IDX));

    // Controls
    ON_CALL(*m_platform_io, push_control("CPU_UNCORE_FREQUENCY_MIN_CONTROL", _, _))
        .WillByDefault(Return(CPU_UNCORE_MIN_CONTROL_IDX));
    ON_CALL(*m_platform_io, push_control("CPU_UNCORE_FREQUENCY_MAX_CONTROL", _, _))
        .WillByDefault(Return(CPU_UNCORE_MAX_CONTROL_IDX));
    ON_CALL(*m_platform_io, agg_function(_))
        .WillByDefault(Return(geopm::Agg::average));

    m_cpu_freq_min = 1000000000.0;
    m_cpu_freq_max = 3700000000.0;
    m_cpu_uncore_freq_min = 1200000000.0;
    m_cpu_uncore_freq_max = 2400000000.0;

    ON_CALL(*m_platform_io, control_domain_type("CPU_FREQUENCY_MAX_CONTROL"))
            .WillByDefault(Return(GEOPM_DOMAIN_CPU));
    ON_CALL(*m_platform_io, signal_domain_type("MSR::CPU_SCALABILITY_RATIO"))
            .WillByDefault(Return(GEOPM_DOMAIN_CPU));
    ON_CALL(*m_platform_topo, is_nested_domain(2, 2)).WillByDefault(Return(true));
    ON_CALL(*m_platform_topo, is_nested_domain(3, 2)).WillByDefault(Return(true));
    ON_CALL(*m_platform_topo, is_nested_domain(2, 3)).WillByDefault(Return(false));
    ON_CALL(*m_platform_topo, is_nested_domain(1, 3)).WillByDefault(Return(false));

    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_freq_min));
    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_freq_max));

    ON_CALL(*m_platform_io, read_signal("CPU_UNCORE_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_uncore_freq_min));
    ON_CALL(*m_platform_io, read_signal("CPU_UNCORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_uncore_freq_max));

    ASSERT_LT(m_cpu_freq_min, 2e9);
    ASSERT_LT(m_cpu_freq_max, 4e9);
    ASSERT_LT(m_cpu_freq_min, m_cpu_freq_max);
    ASSERT_LT(m_cpu_uncore_freq_min, 2e9);
    ASSERT_LT(m_cpu_uncore_freq_max, 3e9);
    ASSERT_LT(m_cpu_uncore_freq_min, m_cpu_uncore_freq_max);

    setup_default_expect_calls();

    m_gov = std::make_shared<MockFrequencyGovernor>();
    ON_CALL(*m_gov, frequency_domain_type())
        .WillByDefault(Return(GEOPM_DOMAIN_CORE));
    EXPECT_CALL(*m_gov, frequency_domain_type()).Times(1);

    EXPECT_CALL(*m_gov, get_frequency_max())
        .WillRepeatedly(Return(m_cpu_freq_max));
    EXPECT_CALL(*m_gov, get_frequency_min())
        .WillRepeatedly(Return(m_cpu_freq_min));

    m_agent = geopm::make_unique<CPUActivityAgent>(*m_platform_io, *m_platform_topo, m_gov, m_waiter);
    m_num_policy = m_agent->policy_names().size();

    m_default_policy = {NAN};

    m_cpu_uncore_freqs = {1.2e9, 1.3e9, 1.4e9, 1.5e9, 1.6e9, 1.7e9, 1.8e9,
                      1.9e9, 2.0e9, 2.1e9, 2.2e9, 2.3e9, 2.4e9};
    m_mbm_max = {45414967307.69231, 64326515384.61539, 72956528846.15384,
                 77349315384.61539, 82345998076.92308, 87738286538.46153,
                 91966364814.81482, 96728174074.07408, 100648379629.62962,
                 102409246296.2963, 103624103703.7037, 104268944444.44444,
                 104748888888.88889};
    ASSERT_EQ(m_cpu_uncore_freqs.size(), m_mbm_max.size());
    ASSERT_EQ(m_mbm_max.size(), M_NUM_UNCORE_MBM_READINGS);

    // Const Config Setup
    std::set<std::string> signal_name_set = {"CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY",
                                             "CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY"};

    ON_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_freq_min));
    ON_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_uncore_freq_min));

    for (size_t i = 0; i < M_NUM_UNCORE_MBM_READINGS; ++i) {
        std::string cc_uncore_freq_name = "CONST_CONFIG::CPU_UNCORE_FREQUENCY_" + std::to_string(i);
        signal_name_set.insert(cc_uncore_freq_name);
        ON_CALL(*m_platform_io, read_signal(cc_uncore_freq_name,
                                            GEOPM_DOMAIN_BOARD, 0)).WillByDefault(Return(m_cpu_uncore_freqs[i]));

        std::string cc_uncore_mem_name = "CONST_CONFIG::CPU_UNCORE_MAX_MEMORY_BANDWIDTH_" + std::to_string(i);
        signal_name_set.insert(cc_uncore_mem_name);
        ON_CALL(*m_platform_io, read_signal(cc_uncore_mem_name,
                                            GEOPM_DOMAIN_BOARD, 0)).WillByDefault(Return(m_mbm_max[i]));
    }

    ON_CALL(*m_platform_io, signal_names()).WillByDefault(Return(signal_name_set));
}

void CPUActivityAgentTest::setup_default_expect_calls()
{
    EXPECT_CALL(*m_platform_io, push_signal("MSR::QM_CTR_SCALED_RATE", _, _)).Times(M_NUM_PACKAGE);
    EXPECT_CALL(*m_platform_io, push_signal("MSR::CPU_SCALABILITY_RATIO", _, _)).Times(M_NUM_CORE);
    EXPECT_CALL(*m_platform_io, push_signal("CPU_UNCORE_FREQUENCY_STATUS", _, _)).Times(M_NUM_PACKAGE);

    EXPECT_CALL(*m_platform_io, push_control("CPU_UNCORE_FREQUENCY_MIN_CONTROL", _, _)).Times(M_NUM_PACKAGE);
    EXPECT_CALL(*m_platform_io, push_control("CPU_UNCORE_FREQUENCY_MAX_CONTROL", _, _)).Times(M_NUM_PACKAGE);

    EXPECT_CALL(*m_platform_io, write_control("MSR::PQR_ASSOC:RMID", _, _, _)).Times(1);
    EXPECT_CALL(*m_platform_io, write_control("MSR::QM_EVTSEL:RMID", _, _, _)).Times(1);
    EXPECT_CALL(*m_platform_io, write_control("MSR::QM_EVTSEL:EVENT_ID", _, _, _)).Times(1);
    EXPECT_CALL(*m_platform_io, read_signal("CPU_UNCORE_FREQUENCY_MIN_CONTROL", _, _)).Times(1);
    EXPECT_CALL(*m_platform_io, read_signal("CPU_UNCORE_FREQUENCY_MAX_CONTROL", _, _)).Times(1);
}


void CPUActivityAgentTest::setup_default_expect_calls_const_config()
{
    for (size_t i = 0; i < M_NUM_UNCORE_MBM_READINGS; ++i) {
        std::string cc_uncore_freq_name = "CONST_CONFIG::CPU_UNCORE_FREQUENCY_" + std::to_string(i);
        std::string cc_uncore_mem_name = "CONST_CONFIG::CPU_UNCORE_MAX_MEMORY_BANDWIDTH_" + std::to_string(i);
        EXPECT_CALL(*m_platform_io, read_signal(cc_uncore_freq_name, _, _)).Times(1);
        EXPECT_CALL(*m_platform_io, read_signal(cc_uncore_mem_name, _, _)).Times(1);
    }

    EXPECT_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", _, _)).Times(1);
    EXPECT_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY", _, _)).Times(1);
}

void CPUActivityAgentTest::TearDown()
{

}

TEST_F(CPUActivityAgentTest, name)
{
    // initialize leaf agent
    setup_default_expect_calls_const_config();
    m_agent->init(0, {}, false);

    EXPECT_EQ("cpu_activity", m_agent->plugin_name());
    EXPECT_NE("bad_string", m_agent->plugin_name());
}

TEST_F(CPUActivityAgentTest, validate_policy)
{
    const std::vector<double> policy_nan(m_num_policy, NAN);
    std::vector<double> policy;

    // initialize leaf agent
    setup_default_expect_calls_const_config();
    m_agent->init(0, {}, false);

    // default policy with 1.2-2.4GHz MBM
    // max rates defined are accepted
    // load default policy
    policy = m_default_policy;
    m_agent->validate_policy(policy);
    // validate policy is unmodified except Phi
    ASSERT_EQ(m_default_policy.size(), policy.size());
    // Default value when NAN is passed is 0.5
    EXPECT_EQ(0.5, policy[PHI]);

    // all-NAN policy is accepted
    // setup & load NAN policy
    policy = policy_nan;
    m_agent->validate_policy(policy);
    // validate policy defaults are applied
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(0.5, policy[PHI]);

    // non-default policy is accepted
    // setup & load policy
    policy[PHI] = 0.1;
    EXPECT_NO_THROW(m_agent->validate_policy(policy));

    // validate policy is modified as expected
    // as phi --> 0 FREQ_EFFICIENT --> FREQ_MAX
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(0.1, policy[PHI]);

    //Policy Phi < 0 --> Error
    policy = policy_nan;
    policy[PHI] = -1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "POLICY_CPU_PHI value out of range");

    //Policy Phi > 1.0 --> Error
    policy = policy_nan;
    policy[PHI] = 1.1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "POLICY_CPU_PHI value out of range");
}

TEST_F(CPUActivityAgentTest, control_signal_granularity_check) {

    ON_CALL(*m_platform_io, signal_domain_type("MSR::CPU_SCALABILITY_RATIO"))
            .WillByDefault(Return(GEOPM_DOMAIN_PACKAGE));

    setup_default_expect_calls_const_config();

    // Extra calls for the granularity check & change
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CORE)).Times(1);
    EXPECT_CALL(*m_gov, frequency_domain_type()).Times(1);

    // See that we actually set to the coarser domain
    EXPECT_CALL(*m_gov, set_domain_type(GEOPM_DOMAIN_PACKAGE)).Times(1);

    m_agent->init(0, {}, false);
}

TEST_F(CPUActivityAgentTest, adjust_platform_high)
{
    // initialize leaf agent
    setup_default_expect_calls_const_config();
    m_agent->init(0, {}, false);

    std::vector<double> policy;
    policy = m_default_policy;
    EXPECT_CALL(*m_gov, validate_policy(m_cpu_freq_min, m_cpu_freq_max)).Times(1);
    m_agent->validate_policy(policy);

    //Sample
    std::vector<double> tmp;
    double mock_active = 1.0;
    EXPECT_CALL(*m_platform_io, sample(CPU_SCALABILITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(QM_CTR_SCALED_RATE_IDX))
                .WillRepeatedly(Return(m_mbm_max.back()));
    EXPECT_CALL(*m_platform_io, sample(CPU_UNCORE_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_cpu_uncore_freq_max));
    m_agent->sample_platform(tmp);

    //Adjust
    //Check frequency
    std::vector<double> freq_vec(M_NUM_CORE, m_cpu_freq_max);
    EXPECT_CALL(*m_gov, adjust_platform(freq_vec)).Times(1);

    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CONTROL_IDX, m_cpu_uncore_freq_max)).Times(M_NUM_PACKAGE);
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CONTROL_IDX, m_cpu_uncore_freq_max)).Times(M_NUM_PACKAGE);
    m_agent->adjust_platform(policy);
    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(CPUActivityAgentTest, adjust_platform_lower_bound_check)
{
    // initialize leaf agent
    setup_default_expect_calls_const_config();
    m_agent->init(0, {}, false);

    std::vector<double> policy;
    policy = m_default_policy;
    EXPECT_CALL(*m_gov, validate_policy(m_cpu_freq_min, m_cpu_freq_max)).Times(1);
    m_agent->validate_policy(policy);

    //Sample
    std::vector<double> tmp;
    double mock_active = 0.5;
    EXPECT_CALL(*m_platform_io, sample(CPU_SCALABILITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(QM_CTR_SCALED_RATE_IDX))
                .WillRepeatedly(Return(m_mbm_max.at(m_mbm_max.size() / 2)));
    EXPECT_CALL(*m_platform_io, sample(CPU_UNCORE_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_cpu_uncore_freq_max - 0.05e9));
    m_agent->sample_platform(tmp);

    double expected_core_freq = m_cpu_freq_min + mock_active *
                                (m_cpu_freq_max - m_cpu_freq_min);
    double expected_uncore_freq = m_cpu_uncore_freq_min +
                                  (m_cpu_uncore_freq_max - m_cpu_uncore_freq_min) *
                                  (m_mbm_max.at(m_mbm_max.size() / 2) /
                                  m_mbm_max.at(m_mbm_max.size() - 2));

    //Adjust
    //Check frequency
    std::vector<double> freq_vec(M_NUM_CORE, expected_core_freq);
    EXPECT_CALL(*m_gov, adjust_platform(freq_vec)).Times(1);

    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CONTROL_IDX,
                                       expected_uncore_freq)).Times(M_NUM_PACKAGE);
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CONTROL_IDX,
                                       expected_uncore_freq)).Times(M_NUM_PACKAGE);
    m_agent->adjust_platform(policy);

    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(CPUActivityAgentTest, adjust_platform_medium)
{
    // initialize leaf agent
    setup_default_expect_calls_const_config();
    m_agent->init(0, {}, false);

    std::vector<double> policy;
    policy = m_default_policy;
    EXPECT_CALL(*m_gov, validate_policy(m_cpu_freq_min, m_cpu_freq_max)).Times(1);
    m_agent->validate_policy(policy);

    //Sample
    std::vector<double> tmp;
    double mock_active = 0.5;
    EXPECT_CALL(*m_platform_io, sample(CPU_SCALABILITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(QM_CTR_SCALED_RATE_IDX))
                .WillRepeatedly(Return(m_mbm_max.at(m_mbm_max.size() / 2)));
    EXPECT_CALL(*m_platform_io, sample(CPU_UNCORE_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_cpu_uncore_freq_max));
    m_agent->sample_platform(tmp);

    double expected_core_freq = m_cpu_freq_min + mock_active *
                                (m_cpu_freq_max - m_cpu_freq_min);
    double expected_uncore_freq = m_cpu_uncore_freq_min +
                                  (m_cpu_uncore_freq_max - m_cpu_uncore_freq_min) *
                                  (m_mbm_max.at(m_mbm_max.size() / 2) /
                                  m_mbm_max.at(m_mbm_max.size() - 1));

    //Adjust
    //Check frequency
    std::vector<double> freq_vec(M_NUM_CORE, expected_core_freq);
    EXPECT_CALL(*m_gov, adjust_platform(freq_vec)).Times(1);

    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CONTROL_IDX,
                                       expected_uncore_freq)).Times(M_NUM_PACKAGE);
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CONTROL_IDX,
                                       expected_uncore_freq)).Times(M_NUM_PACKAGE);
    m_agent->adjust_platform(policy);

    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(CPUActivityAgentTest, adjust_platform_low)
{
    // initialize leaf agent
    setup_default_expect_calls_const_config();
    m_agent->init(0, {}, false);

    std::vector<double> policy;
    policy = m_default_policy;
    EXPECT_CALL(*m_gov, validate_policy(m_cpu_freq_min, m_cpu_freq_max)).Times(1);
    m_agent->validate_policy(policy);

    //Sample
    std::vector<double> tmp;
    double mock_active = 0.1;
    EXPECT_CALL(*m_platform_io, sample(CPU_SCALABILITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(QM_CTR_SCALED_RATE_IDX))
                .WillRepeatedly(Return(m_mbm_max.at(2)));
    EXPECT_CALL(*m_platform_io, sample(CPU_UNCORE_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_cpu_uncore_freq_max));
    m_agent->sample_platform(tmp);

    double expected_core_freq = m_cpu_freq_min + mock_active *
                                (m_cpu_freq_max - m_cpu_freq_min);
    double expected_uncore_freq = m_cpu_uncore_freq_min +
                                  (m_cpu_uncore_freq_max - m_cpu_uncore_freq_min) *
                                  (m_mbm_max.at(2) /
                                  m_mbm_max.at(m_mbm_max.size() - 1));

    //Adjust
    //Check frequency
    std::vector<double> freq_vec(M_NUM_CORE, expected_core_freq);
    EXPECT_CALL(*m_gov, adjust_platform(freq_vec)).Times(1);

    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CONTROL_IDX,
                                       expected_uncore_freq)).Times(M_NUM_PACKAGE);
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CONTROL_IDX,
                                       expected_uncore_freq)).Times(M_NUM_PACKAGE);
    m_agent->adjust_platform(policy);

    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(CPUActivityAgentTest, adjust_platform_zero)
{
    // initialize leaf agent
    setup_default_expect_calls_const_config();
    m_agent->init(0, {}, false);

    std::vector<double> policy;
    policy = m_default_policy;
    EXPECT_CALL(*m_gov, validate_policy(m_cpu_freq_min, m_cpu_freq_max)).Times(1);
    m_agent->validate_policy(policy);

    //Sample
    std::vector<double> tmp;
    double mock_active = 0.0;
    EXPECT_CALL(*m_platform_io, sample(CPU_SCALABILITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(QM_CTR_SCALED_RATE_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(CPU_UNCORE_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_cpu_uncore_freq_max));
    m_agent->sample_platform(tmp);

    //Adjust
    //Check frequency
    std::vector<double> freq_vec(M_NUM_CORE, m_cpu_freq_min);
    EXPECT_CALL(*m_gov, adjust_platform(freq_vec)).Times(1);

    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CONTROL_IDX, m_cpu_uncore_freq_min)).Times(M_NUM_PACKAGE);
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CONTROL_IDX, m_cpu_uncore_freq_min)).Times(M_NUM_PACKAGE);
    m_agent->adjust_platform(policy);
    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(CPUActivityAgentTest, adjust_platform_signal_out_of_bounds)
{
    // initialize leaf agent
    setup_default_expect_calls_const_config();
    m_agent->init(0, {}, false);

    std::vector<double> policy;
    policy = m_default_policy;
    m_agent->validate_policy(policy);

    //Sample
    std::vector<double> tmp;
    double mock_active = 1e99;
    EXPECT_CALL(*m_platform_io, sample(CPU_SCALABILITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(QM_CTR_SCALED_RATE_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(CPU_UNCORE_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_cpu_uncore_freq_max));
    m_agent->sample_platform(tmp);

    //Adjust
    //Check call was made.  It's the frequency governor responsibility to handle
    //clamping.
    EXPECT_CALL(*m_gov, validate_policy(m_cpu_freq_min, m_cpu_freq_max)).Times(1);
    EXPECT_CALL(*m_gov, adjust_platform(_)).Times(1);
    //Check frequency
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CONTROL_IDX, m_cpu_uncore_freq_max)).Times(M_NUM_PACKAGE);
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CONTROL_IDX, m_cpu_uncore_freq_max)).Times(M_NUM_PACKAGE);
    m_agent->adjust_platform(policy);
    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());

    //Sample
    mock_active = -1;
    EXPECT_CALL(*m_platform_io, sample(CPU_SCALABILITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(QM_CTR_SCALED_RATE_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(CPU_UNCORE_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_cpu_uncore_freq_max));
    m_agent->sample_platform(tmp);

    //Adjust
    //Check call was made.  It's the frequency governor responsibility to handle
    //clamping.
    EXPECT_CALL(*m_gov, validate_policy(m_cpu_freq_min, m_cpu_freq_max)).Times(1);
    EXPECT_CALL(*m_gov, adjust_platform(_)).Times(1);
    //Check frequency
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CONTROL_IDX, m_cpu_uncore_freq_min)).Times(M_NUM_PACKAGE);
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CONTROL_IDX, m_cpu_uncore_freq_min)).Times(M_NUM_PACKAGE);
    m_agent->adjust_platform(policy);
    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(CPUActivityAgentTest, no_mem_constconfig)
{
    std::set<std::string> signal_name_set = {"CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY",
                                             "CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY"};

    ON_CALL(*m_platform_io, signal_names()).WillByDefault(Return(signal_name_set));
    // init leaf agent expecting failure
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->init(0, {}, false), GEOPM_ERROR_INVALID,
                                "ConstConfigIOGroup configuration file does not contain memory bandwidth information");
}


TEST_F(CPUActivityAgentTest, invalid_fe_core_low)
{
    EXPECT_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", _, _)).Times(1);
    EXPECT_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY", _, _)).Times(1);

    ON_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_freq_min - 1));
    // init leaf agent expecting failure
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->init(0, {}, false), GEOPM_ERROR_INVALID,
                                "(): Core efficient frequency out of range: ");
}

TEST_F(CPUActivityAgentTest, invalid_fe_core_high)
{
    EXPECT_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", _, _)).Times(1);
    EXPECT_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY", _, _)).Times(1);

    ON_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_freq_max + 1));
    // init leaf agent expecting failure
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->init(0, {}, false), GEOPM_ERROR_INVALID,
                                "(): Core efficient frequency out of range: ");
}

TEST_F(CPUActivityAgentTest, invalid_fe_uncore_low)
{
    EXPECT_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", _, _)).Times(1);
    EXPECT_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY", _, _)).Times(1);

    ON_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_uncore_freq_min - 1));
    // init leaf agent expecting failure
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->init(0, {}, false), GEOPM_ERROR_INVALID,
                                "(): Uncore efficient frequency out of range: ");
}

TEST_F(CPUActivityAgentTest, invalid_fe_uncore_high)
{
    EXPECT_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", _, _)).Times(1);
    EXPECT_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY", _, _)).Times(1);

    ON_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_freq_min));
    ON_CALL(*m_platform_io, read_signal("CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(m_cpu_uncore_freq_max + 1));
    // init leaf agent expecting failure
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->init(0, {}, false), GEOPM_ERROR_INVALID,
                                "(): Uncore efficient frequency out of range: ");
}

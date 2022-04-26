/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
#include "GPUActivityAgent.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_prof.h"
#include "geopm_test.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Sequence;
using ::testing::Return;
using ::testing::AtLeast;
using geopm::GPUActivityAgent;
using geopm::PlatformTopo;

class GPUActivityAgentTest : public :: testing :: Test
{
    protected:
        enum mock_pio_idx_e {
            GPU_FREQUENCY_IDX,
            GPU_COMPUTE_ACTIVITY_IDX,
            GPU_UTILIZATION_IDX,
            GPU_ENERGY_IDX,
            GPU_FREQUENCY_CONTROL_IDX,
        };
        enum policy_idx_e {
            FREQ_MAX = 0,
            FREQ_EFFICIENT = 1,
            PHI = 2,
            PERIOD = 3,
        };

        void SetUp();
        void TearDown();
        static const int M_NUM_CPU = 1;
        static const int M_NUM_BOARD = 1;
        static const int M_NUM_BOARD_ACCELERATOR = 1;
        std::unique_ptr<GPUActivityAgent> m_agent;
        std::vector<double> m_default_policy;
        size_t m_num_policy;
        double m_freq_min;
        double m_freq_max;
        double m_sample_period;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
};

void GPUActivityAgentTest::SetUp()
{
    m_platform_io = geopm::make_unique<MockPlatformIO>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();

    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(M_NUM_BOARD));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR))
        .WillByDefault(Return(M_NUM_BOARD_ACCELERATOR));

    ON_CALL(*m_platform_io, push_signal("GPU_FREQUENCY_STATUS", _, _))
        .WillByDefault(Return(GPU_FREQUENCY_IDX));
    ON_CALL(*m_platform_io, push_signal("GPU_COMPUTE_ACTIVITY", _, _))
        .WillByDefault(Return(GPU_COMPUTE_ACTIVITY_IDX));
    ON_CALL(*m_platform_io, push_signal("GPU_UTILIZATION", _, _))
        .WillByDefault(Return(GPU_UTILIZATION_IDX));
    ON_CALL(*m_platform_io, push_signal("GPU_ENERGY", _, _))
        .WillByDefault(Return(GPU_ENERGY_IDX));
    ON_CALL(*m_platform_io, push_control("GPU_FREQUENCY_CONTROL", _, _))
        .WillByDefault(Return(GPU_FREQUENCY_CONTROL_IDX));
    ON_CALL(*m_platform_io, agg_function(_))
        .WillByDefault(Return(geopm::Agg::average));

    m_freq_min = 0135000000.0;
    m_freq_max = 1530000000.0;
    ON_CALL(*m_platform_io, control_domain_type("GPU_FREQUENCY_CONTROL"))
        .WillByDefault(Return(GEOPM_DOMAIN_BOARD_ACCELERATOR));
    ON_CALL(*m_platform_io, signal_domain_type("GPU_COMPUTE_ACTIVITY"))
        .WillByDefault(Return(GEOPM_DOMAIN_BOARD_ACCELERATOR));
    ON_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_min));
    ON_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_max));

    ASSERT_LT(m_freq_min, 0.2e9);
    ASSERT_LT(1.4e9, m_freq_max);

    m_agent = geopm::make_unique<GPUActivityAgent>(*m_platform_io, *m_platform_topo);
    m_num_policy = m_agent->policy_names().size();

    m_sample_period = 0.020;

    m_default_policy = {m_freq_max, m_freq_min, NAN, NAN};

    // leaf agent
    m_agent->init(0, {}, false);
}

void GPUActivityAgentTest::TearDown()
{

}

TEST_F(GPUActivityAgentTest, name)
{
    EXPECT_EQ("gpu_activity", m_agent->plugin_name());
    EXPECT_NE("bad_string", m_agent->plugin_name());
}

TEST_F(GPUActivityAgentTest, validate_policy)
{
    //Called as part of validate
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MIN_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_min));
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MAX_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_max));
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD)).WillRepeatedly(Return(M_NUM_BOARD));

    const std::vector<double> empty(m_num_policy, NAN);
    std::vector<double> policy;

    // default policy is accepted
    // load default policy
    policy = m_default_policy;
    m_agent->validate_policy(policy);
    // validate policy is unmodified except Phi
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(m_freq_max, policy[FREQ_MAX]);
    EXPECT_EQ(m_freq_min, policy[FREQ_EFFICIENT]);
    // Default value when NAN is passed is 0.5
    EXPECT_EQ(0.5, policy[PHI]);
    EXPECT_EQ(m_sample_period, policy[PERIOD]);

    // all-NAN policy is accepted
    // setup & load NAN policy
    policy = empty;
    m_agent->validate_policy(policy);
    // validate policy defaults are applied
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(m_freq_max, policy[FREQ_MAX]);
    EXPECT_EQ((policy[FREQ_MAX] + m_freq_min) / 2, policy[FREQ_EFFICIENT]);
    EXPECT_EQ(0.5, policy[PHI]);
    EXPECT_EQ(m_sample_period, policy[PERIOD]);

    // non-default policy is accepted
    // setup & load policy
    policy[FREQ_MAX] = m_freq_max;
    policy[FREQ_EFFICIENT] = m_freq_max / 2;
    policy[PHI] = 0.1;
    policy[PERIOD] = 0.005;
    m_agent->validate_policy(policy);

    // validate policy is modified as expected
    // as phi --> 0 FREQ_EFFICIENT --> FREQ_MAX
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(m_freq_max, policy[FREQ_MAX]);
    EXPECT_GE(policy[FREQ_EFFICIENT], m_freq_max / 2);
    EXPECT_LE(policy[FREQ_EFFICIENT], m_freq_max);
    EXPECT_EQ(0.1, policy[PHI]);
    EXPECT_EQ(0.005, policy[PERIOD]);

    //Fe > Fmax --> Error
    policy[FREQ_MAX] = NAN;
    policy[FREQ_EFFICIENT] = m_freq_max + 1;
    policy[PHI] = NAN;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                              "GPU_FREQ_EFFICIENT out of range");

    //Fe < Fmin --> Error
    policy[FREQ_MAX] = NAN;
    policy[FREQ_EFFICIENT] = m_freq_min - 1;
    policy[PHI] = NAN;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "GPU_FREQ_EFFICIENT out of range");

    //Fe > Policy Fmax --> Error
    policy[FREQ_MAX] = m_freq_max - 2;
    policy[FREQ_EFFICIENT] = m_freq_max - 1;
    policy[PHI] = NAN;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "value exceeds GPU_FREQ_MAX");

    //Policy Fmax > Fmax --> Error
    policy[FREQ_MAX] = m_freq_max + 1;
    policy[FREQ_EFFICIENT] = NAN;
    policy[PHI] = NAN;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "GPU_FREQ_MAX out of range");

    //Policy Fmax < Fmin --> Error
    policy[FREQ_MAX] = m_freq_min - 1;
    policy[FREQ_EFFICIENT] = NAN;
    policy[PHI] = NAN;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "GPU_FREQ_MAX out of range");

    //Policy Phi < 0 --> Error
    policy[FREQ_MAX] = NAN;
    policy[FREQ_EFFICIENT] = NAN;
    policy[PHI] = -1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "POLICY_GPU_PHI value out of range");

    //Policy Phi > 1.0 --> Error
    policy[FREQ_MAX] = NAN;
    policy[FREQ_EFFICIENT] = NAN;
    policy[PHI] = 1.1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "POLICY_GPU_PHI value out of range");

    policy[FREQ_MAX] = NAN;
    policy[FREQ_EFFICIENT] = NAN;
    policy[PHI] = 1.0;
    policy[PERIOD] = -1.0;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "Sample period must be greater than 0.");

}

TEST_F(GPUActivityAgentTest, adjust_platform_high)
{
    //TODO: Setup f_max, min etc...via read
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MIN_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_min));
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MAX_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_max));
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD)).WillRepeatedly(Return(M_NUM_BOARD));
    std::vector<double> policy;
    policy = m_default_policy;
    m_agent->validate_policy(policy);

    //Sample
    std::vector<double> tmp;
    double mock_active = 1.0;
    double mock_util = 1.0;
    EXPECT_CALL(*m_platform_io, sample(GPU_COMPUTE_ACTIVITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                .WillRepeatedly(Return(mock_util));
    EXPECT_CALL(*m_platform_io, sample(GPU_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_freq_max - 1)); //Any non-m_freq_max value will work here
    EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                .WillRepeatedly(Return(123456789));
    m_agent->sample_platform(tmp);

    //Adjust
    //Check frequency
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_IDX, m_freq_max)).Times(M_NUM_BOARD_ACCELERATOR);
    m_agent->adjust_platform(policy);

    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(GPUActivityAgentTest, adjust_platform_medium)
{
    //TODO: Setup f_max, min etc...via read
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MIN_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_min));
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MAX_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_max));
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD)).WillRepeatedly(Return(M_NUM_BOARD));
    std::vector<double> policy;
    policy = m_default_policy;
    m_agent->validate_policy(policy);

    //Sample
    std::vector<double> tmp;
    double mock_active = 0.5;
    double mock_util = 1.0;
    EXPECT_CALL(*m_platform_io, sample(GPU_COMPUTE_ACTIVITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                .WillRepeatedly(Return(mock_util));
    EXPECT_CALL(*m_platform_io, sample(GPU_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_freq_max-1)); //Any non-m_freq_max value will work here
    EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                .WillRepeatedly(Return(123456789));
    m_agent->sample_platform(tmp);

    //Adjust
    //Check frequency
    double expected_freq = policy[FREQ_EFFICIENT] + (m_freq_max - policy[FREQ_EFFICIENT]) * mock_active;
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_IDX, expected_freq)).Times(M_NUM_BOARD_ACCELERATOR);

    m_agent->adjust_platform(policy);

    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(GPUActivityAgentTest, adjust_platform_low)
{
    //TODO: Setup f_max, min etc...via read
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MIN_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_min));
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MAX_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_max));
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD)).WillRepeatedly(Return(M_NUM_BOARD));
    std::vector<double> policy;
    policy = m_default_policy;
    m_agent->validate_policy(policy);

    //Sample
    std::vector<double> tmp;
    double mock_active = 0.1;
    double mock_util = 1.0;
    EXPECT_CALL(*m_platform_io, sample(GPU_COMPUTE_ACTIVITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                .WillRepeatedly(Return(mock_util));
    EXPECT_CALL(*m_platform_io, sample(GPU_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_freq_max-1)); //Any non-m_freq_max value will work here

    EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                .WillRepeatedly(Return(123456789));
    m_agent->sample_platform(tmp);

    //Adjust
    //Check frequency
    double expected_freq = policy[FREQ_EFFICIENT] + (m_freq_max - policy[FREQ_EFFICIENT]) * mock_active;
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_IDX, expected_freq)).Times(M_NUM_BOARD_ACCELERATOR);
    m_agent->adjust_platform(policy);
    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(GPUActivityAgentTest, adjust_platform_zero)
{
    //TODO: Setup f_max, min etc...via read
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MIN_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_min));
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MAX_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_max));
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD)).WillRepeatedly(Return(M_NUM_BOARD));
    std::vector<double> policy;
    policy = m_default_policy;
    m_agent->validate_policy(policy);


    //Sample
    std::vector<double> tmp;
    double mock_active = 0.0;
    double mock_util = 1.0;
    EXPECT_CALL(*m_platform_io, sample(GPU_COMPUTE_ACTIVITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                .WillRepeatedly(Return(mock_util));
    EXPECT_CALL(*m_platform_io, sample(GPU_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_freq_max)); //Any non-m_freq_min value will work here
    EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                .WillRepeatedly(Return(123456789));
    m_agent->sample_platform(tmp);

    //Adjust
    //Check frequency
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_IDX, m_freq_min)).Times(M_NUM_BOARD_ACCELERATOR);
    m_agent->adjust_platform(policy);

    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());

}

TEST_F(GPUActivityAgentTest, adjust_platform_signal_out_of_bounds)
{
    //TODO: Setup f_max, min etc...via read
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MIN_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_min));
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MAX_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_max));
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD)).WillRepeatedly(Return(M_NUM_BOARD));
    std::vector<double> policy;
    policy = m_default_policy;
    m_agent->validate_policy(policy);

    //Sample
    std::vector<double> tmp;
    double mock_active = 987654321;
    double mock_util = 1.0;
    EXPECT_CALL(*m_platform_io, sample(GPU_COMPUTE_ACTIVITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                .WillRepeatedly(Return(mock_util));
    EXPECT_CALL(*m_platform_io, sample(GPU_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_freq_max)); //Any non-m_freq_min value will work here
    EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                .WillRepeatedly(Return(123456789));
    m_agent->sample_platform(tmp);

    //Adjust
    //Check frequency
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_IDX, m_freq_max)).Times(M_NUM_BOARD_ACCELERATOR);
    m_agent->adjust_platform(policy);

    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());

    //Sample
    mock_active = -12345;
    mock_util = 1.0;
    EXPECT_CALL(*m_platform_io, sample(GPU_COMPUTE_ACTIVITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                .WillRepeatedly(Return(mock_util));
    EXPECT_CALL(*m_platform_io, sample(GPU_FREQUENCY_IDX))
                .WillRepeatedly(Return(m_freq_max)); //Any non-m_freq_min value will work here
    EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                .WillRepeatedly(Return(123456789));
    m_agent->sample_platform(tmp);

    //Adjust
    //Check frequency
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_IDX, m_freq_min)).Times(M_NUM_BOARD_ACCELERATOR);
    m_agent->adjust_platform(policy);

    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());

}

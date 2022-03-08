/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include "config.h"

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_agent.h"
#include "geopm_internal.h"
#include "geopm_hash.h"

#include "Agent.hpp"
#include "FixedFrequencyAgent.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm.h"
#include "geopm_test.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Sequence;
using ::testing::Return;
using ::testing::AtLeast;
using ::testing::AnyNumber;
using geopm::FixedFrequencyAgent;
using geopm::PlatformTopo;

class FixedFrequencyAgentTest : public :: testing :: Test
{
    protected:
      enum mock_pio_idx_e {
           FREQUENCY_GPU_CONTROL_IDX,
           FREQ_CONTROL_IDX,
           UNCORE_MIN_CTL_IDX,
           UNCORE_MAX_CTL_IDX,
           SAMPLE_PERIOD_IDX
        };
        enum policy_idx_e {
             GPU_FREQUENCY = 0, 
             CPU_FREQUENCY = 1,
             UNCORE_MIN_FREQUENCY = 2,
             UNCORE_MAX_FREQUENCY = 3,
             SAMPLE_PERIOD = 4
        };

        void SetUp();
        void TearDown();
        static const int M_NUM_CPU = 1;
        static const int M_NUM_BOARD = 1;
        static const int M_NUM_BOARD_ACCELERATOR = 1; //TODO: Increase
        std::unique_ptr<FixedFrequencyAgent> m_agent;
        std::vector<double> m_default_policy;
        size_t m_num_policy;
        double m_freq_gpu_min;
        double m_freq_gpu_max;
        double m_freq_min;
        double m_freq_max;
        double m_freq_uncore_min;
        double m_freq_uncore_max;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
};

void FixedFrequencyAgentTest::SetUp()
{
    m_platform_io = geopm::make_unique<MockPlatformIO>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();

    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(M_NUM_BOARD));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR))
        .WillByDefault(Return(M_NUM_BOARD_ACCELERATOR));

    ON_CALL(*m_platform_io, push_control("FREQUENCY_GPU_CONTROL", _, _))
        .WillByDefault(Return(FREQUENCY_GPU_CONTROL_IDX));
    ON_CALL(*m_platform_io, push_control("FREQUENCY", _, _))
        .WillByDefault(Return(FREQ_CONTROL_IDX));
    ON_CALL(*m_platform_io, push_control("CPU_FREQUENCY_CONTROL", _, _))
        .WillByDefault(Return(FREQ_CONTROL_IDX));
    ON_CALL(*m_platform_io, push_control("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", _, _))
        .WillByDefault(Return(UNCORE_MIN_CTL_IDX));
    ON_CALL(*m_platform_io, push_control("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", _, _))
        .WillByDefault(Return(UNCORE_MAX_CTL_IDX));
    ON_CALL(*m_platform_io, agg_function(_))
        .WillByDefault(Return(geopm::Agg::average));

    m_freq_gpu_min = 0135000000.0;
    m_freq_gpu_max = 1530000000.0;
    ON_CALL(*m_platform_io, control_domain_type("FREQUENCY_GPU_CONTROL"))
        .WillByDefault(Return(GEOPM_DOMAIN_BOARD_ACCELERATOR));
    ON_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_gpu_min));
    ON_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_gpu_max));

    ASSERT_LT(m_freq_gpu_min, 0.2e9);
    ASSERT_LT(1.4e9, m_freq_gpu_max);

    m_freq_min = 1800000000.0;
    m_freq_max = 2200000000.0;
    m_freq_uncore_min = 1700000000;
    m_freq_uncore_max = 2100000000;

    ON_CALL(*m_platform_io, read_signal("FREQUENCY_MIN", GEOPM_DOMAIN_BOARD, 0))
      .WillByDefault(Return(m_freq_min));
    ON_CALL(*m_platform_io, read_signal("FREQUENCY_MAX", GEOPM_DOMAIN_BOARD, 0))
      .WillByDefault(Return(m_freq_max));
    ON_CALL(*m_platform_io, read_signal("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_uncore_min));
    ON_CALL(*m_platform_io, read_signal("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_uncore_max));

    ASSERT_LT(m_freq_min, 1.9e9);
    ASSERT_LT(2.1e9, m_freq_max);
    ASSERT_LT(m_freq_uncore_min, 1.9e9);
    ASSERT_LT(2.0e9, m_freq_uncore_max);
    
    m_agent = geopm::make_unique<FixedFrequencyAgent>(*m_platform_io, *m_platform_topo);
    m_num_policy = m_agent->policy_names().size();

    m_default_policy = {m_freq_gpu_max, m_freq_max, m_freq_uncore_min, m_freq_uncore_max, 0.05};

    // leaf agent
    m_agent->init(0, {}, false);
}

void FixedFrequencyAgentTest::TearDown()
{

}

TEST_F(FixedFrequencyAgentTest, name)
{
    EXPECT_EQ("fixed_frequency", m_agent->plugin_name());
    EXPECT_NE("bad_string", m_agent->plugin_name());
}

TEST_F(FixedFrequencyAgentTest, validate_policy)
{
    const std::vector<double> empty(m_num_policy, NAN);
    std::vector<double> policy;

    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MIN_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_gpu_min));
    EXPECT_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MAX_AVAIL", _, _)).WillRepeatedly(
                Return(m_freq_gpu_max));
    EXPECT_CALL(*m_platform_io, read_signal("FREQUENCY_MIN", _, _)).WillRepeatedly(
                Return(m_freq_min));
    EXPECT_CALL(*m_platform_io, read_signal("FREQUENCY_MAX", _, _)).WillRepeatedly(
                Return(m_freq_max));


    // default policy is accepted
    // load default policy
    policy = m_default_policy;
    m_agent->validate_policy(policy);
    // validate policy is unmodified 
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(m_freq_gpu_max, policy[GPU_FREQUENCY]);
    EXPECT_EQ(m_freq_max, policy[CPU_FREQUENCY]);
    EXPECT_EQ(m_freq_uncore_min, policy[UNCORE_MIN_FREQUENCY]);
    EXPECT_EQ(m_freq_uncore_max, policy[UNCORE_MAX_FREQUENCY]);

    // all-NAN policy is accepted
    // setup & load NAN policy
    policy = empty;
    m_agent->validate_policy(policy);
    EXPECT_TRUE(std::isnan(policy[GPU_FREQUENCY]));

    // check for gpu frequency out of range
    policy[GPU_FREQUENCY] = m_freq_gpu_max + 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
			       "gpu frequency out of range");

    policy[GPU_FREQUENCY] = m_freq_gpu_min - 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
			       "gpu frequency out of range");

    // check for cpu frequency out of range
    policy = empty;
    policy[CPU_FREQUENCY] = m_freq_max + 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
			       "cpu frequency out of range");

    policy[CPU_FREQUENCY] = m_freq_min - 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
			       "cpu frequency out of range");

    // check for uncore frequency min <= max
    policy = empty;
    policy[UNCORE_MIN_FREQUENCY] = m_freq_uncore_max;
    policy[UNCORE_MAX_FREQUENCY] = m_freq_uncore_min;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
			       "min uncore frequency cannot be larger than max uncore frequency");

    // check period is greater than 0
    policy = empty;
    policy[SAMPLE_PERIOD] = 0.0;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
			       "sample period must be greater than 0");
    

}

TEST_F(FixedFrequencyAgentTest, adjust_platform)
{
    const std::vector<double> empty(m_num_policy, NAN);
    std::vector<double> policy;

    policy = empty;
    m_agent->adjust_platform(policy);
    EXPECT_FALSE(m_agent->do_write_batch());

    policy = m_default_policy;
    m_agent->adjust_platform(policy);
    EXPECT_FALSE(m_agent->do_write_batch());
    

    m_agent->adjust_platform(policy);
    EXPECT_FALSE(m_agent->do_write_batch());
    
}


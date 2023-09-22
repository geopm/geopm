/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <stdlib.h>
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
#include "MockWaiter.hpp"
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
            GPU_CORE_ACTIVITY_IDX,
            GPU_UTILIZATION_IDX,
            GPU_ENERGY_IDX,
            GPU_FREQUENCY_CONTROL_MIN_IDX,
            GPU_FREQUENCY_CONTROL_MAX_IDX,
            TIME_IDX
        };
        enum policy_idx_e {
            PHI = 0
        };

        void SetUp();
        void TearDown();
        void set_up_val_policy_expectations();

        void test_adjust_platform(std::vector<double> &policy,
                                  double mock_active,
                                  double mock_util,
                                  double expected_freq);
        static const int M_NUM_CPU;
        static const int M_NUM_BOARD;
        static const int M_NUM_GPU;
        static const int M_NUM_GPU_CHIP;
        static const double M_FREQ_MIN;
        static const double M_FREQ_MAX;
        static const double M_FREQ_EFFICIENT;
        static const std::vector<double> M_DEFAULT_POLICY;
        size_t m_num_policy;
        std::set<std::string> empty_set;
        std::unique_ptr<GPUActivityAgent> m_agent;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
        std::shared_ptr<MockWaiter> m_waiter;
};

const int GPUActivityAgentTest::M_NUM_CPU = 1;
const int GPUActivityAgentTest::M_NUM_BOARD = 1;
const int GPUActivityAgentTest::M_NUM_GPU = 1;
const int GPUActivityAgentTest::M_NUM_GPU_CHIP = 1;
const double GPUActivityAgentTest::M_FREQ_MIN = 0135000000.0;
const double GPUActivityAgentTest::M_FREQ_MAX = 1530000000.0;
const double GPUActivityAgentTest::M_FREQ_EFFICIENT = (M_FREQ_MIN + M_FREQ_MAX) / 2;
const std::vector<double> GPUActivityAgentTest::M_DEFAULT_POLICY = {NAN};

void GPUActivityAgentTest::SetUp()
{
    m_platform_io = std::make_unique<MockPlatformIO>();
    m_platform_topo = std::make_unique<MockPlatformTopo>();
    m_waiter = std::make_unique<MockWaiter>();

    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(M_NUM_BOARD));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
        .WillByDefault(Return(M_NUM_GPU));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU_CHIP))
        .WillByDefault(Return(M_NUM_GPU_CHIP));

    ON_CALL(*m_platform_io, push_signal("GPU_CORE_ACTIVITY", _, _))
        .WillByDefault(Return(GPU_CORE_ACTIVITY_IDX));
    ON_CALL(*m_platform_io, push_signal("GPU_UTILIZATION", _, _))
        .WillByDefault(Return(GPU_UTILIZATION_IDX));
    ON_CALL(*m_platform_io, push_signal("GPU_ENERGY", _, _))
        .WillByDefault(Return(GPU_ENERGY_IDX));
    ON_CALL(*m_platform_io, push_signal("TIME", _, _))
        .WillByDefault(Return(TIME_IDX));

    ON_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MIN_CONTROL", _, _))
        .WillByDefault(Return(GPU_FREQUENCY_CONTROL_MIN_IDX));
    ON_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MAX_CONTROL", _, _))
        .WillByDefault(Return(GPU_FREQUENCY_CONTROL_MAX_IDX));
    ON_CALL(*m_platform_io, agg_function(_))
        .WillByDefault(Return(geopm::Agg::average));

    ON_CALL(*m_platform_io, control_domain_type("GPU_CORE_FREQUENCY_MIN_CONTROL"))
        .WillByDefault(Return(GEOPM_DOMAIN_GPU_CHIP));
    ON_CALL(*m_platform_io, control_domain_type("GPU_CORE_FREQUENCY_MAX_CONTROL"))
        .WillByDefault(Return(GEOPM_DOMAIN_GPU_CHIP));
    ON_CALL(*m_platform_io, signal_domain_type("GPU_CORE_ACTIVITY"))
        .WillByDefault(Return(GEOPM_DOMAIN_GPU_CHIP));
    ON_CALL(*m_platform_io, signal_domain_type("GPU_CORE_FREQUENCY_STATUS"))
        .WillByDefault(Return(GEOPM_DOMAIN_GPU_CHIP));
    ON_CALL(*m_platform_io, signal_domain_type("GPU_UTILIZATION"))
        .WillByDefault(Return(GEOPM_DOMAIN_GPU_CHIP));

    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_FREQ_MIN));
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_FREQ_MAX));

    ON_CALL(*m_platform_io, signal_names())
        .WillByDefault(Return(empty_set));

    ASSERT_LT(M_FREQ_MIN, 0.2e9);
    ASSERT_LT(1.4e9, M_FREQ_MAX);

    std::set<std::string> signal_name_set = {"CONST_CONFIG::GPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY"};
    ON_CALL(*m_platform_io, read_signal("CONST_CONFIG::GPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(M_FREQ_EFFICIENT));

    ON_CALL(*m_platform_io, signal_names()).WillByDefault(Return(signal_name_set));

    m_agent = geopm::make_unique<GPUActivityAgent>(*m_platform_io, *m_platform_topo, m_waiter);
    m_num_policy = m_agent->policy_names().size();

    // leaf agent
    m_agent->init(0, {}, false);
}

void GPUActivityAgentTest::TearDown()
{
}

void GPUActivityAgentTest::set_up_val_policy_expectations()
{
    EXPECT_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", _, _)).WillRepeatedly(
                Return(M_FREQ_MIN));
    EXPECT_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", _, _)).WillRepeatedly(
                Return(M_FREQ_MAX));
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD)).WillRepeatedly(Return(M_NUM_BOARD));
}

TEST_F(GPUActivityAgentTest, name)
{
    EXPECT_EQ("gpu_activity", m_agent->plugin_name());
    EXPECT_NE("bad_string", m_agent->plugin_name());
}

TEST_F(GPUActivityAgentTest, validate_policy)
{
    set_up_val_policy_expectations();

    const std::vector<double> empty(m_num_policy, NAN);
    EXPECT_CALL(*m_platform_io, signal_names()).WillRepeatedly(Return(empty_set));

    // default policy is accepted
    // load default policy
    std::vector<double> policy = M_DEFAULT_POLICY;
    EXPECT_NO_THROW(m_agent->validate_policy(policy));
    // validate policy is unmodified except Phi
    ASSERT_EQ(m_num_policy, policy.size());
    // Default value when NAN is passed is 0.5
    EXPECT_EQ(0.5, policy[PHI]);

    // all-NAN policy is accepted
    // setup & load NAN policy
    policy = empty;
    EXPECT_NO_THROW(m_agent->validate_policy(policy));
    // validate policy defaults are applied
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(0.5, policy[PHI]);

    // non-default policy is accepted
    // setup & load policy
    EXPECT_CALL(*m_platform_io, signal_names()).WillRepeatedly(Return(empty_set));
    policy[PHI] = 0.1;
    EXPECT_NO_THROW(m_agent->validate_policy(policy));

    //Policy Phi < 0 --> Error
    policy[PHI] = -1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "POLICY_GPU_PHI value out of range");

    //Policy Phi > 1.0 --> Error
    policy[PHI] = 1.1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "POLICY_GPU_PHI value out of range");
}

void GPUActivityAgentTest::test_adjust_platform(std::vector<double> &policy,
                                                double mock_active,
                                                double mock_util,
                                                double expected_freq)
{
    set_up_val_policy_expectations();

    EXPECT_NO_THROW(m_agent->validate_policy(policy));

    //Sample
    std::vector<double> tmp;
    EXPECT_CALL(*m_platform_io, sample(GPU_CORE_ACTIVITY_IDX))
                .WillRepeatedly(Return(mock_active));
    EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                .WillRepeatedly(Return(mock_util));
    EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                .WillRepeatedly(Return(123456789));
    EXPECT_CALL(*m_platform_io, sample(TIME_IDX))
                .Times(1);
    m_agent->sample_platform(tmp);

    //Adjust
    //Check frequency
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MIN_IDX, expected_freq)).Times(M_NUM_GPU_CHIP);
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MAX_IDX, expected_freq)).Times(M_NUM_GPU_CHIP);

    m_agent->adjust_platform(policy);

    //Check a frequency decision resulted in write batch being true
    EXPECT_TRUE(m_agent->do_write_batch());
}

TEST_F(GPUActivityAgentTest, adjust_platform_high)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = 1.0;
    double mock_util = 1.0;
    test_adjust_platform(policy, mock_active, mock_util, M_FREQ_MAX);
}

TEST_F(GPUActivityAgentTest, adjust_platform_medium)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = 0.5;
    double mock_util = 1.0;
    double expected_freq = M_FREQ_EFFICIENT +
            (M_FREQ_MAX - M_FREQ_EFFICIENT) * mock_active;
    test_adjust_platform(policy, mock_active, mock_util, expected_freq);
}

TEST_F(GPUActivityAgentTest, adjust_platform_low)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = 0.1;
    double mock_util = 1.0;
    double expected_freq = M_FREQ_EFFICIENT +
            (M_FREQ_MAX - M_FREQ_EFFICIENT) * mock_active;
    test_adjust_platform(policy, mock_active, mock_util, expected_freq);
}

TEST_F(GPUActivityAgentTest, adjust_platform_zero)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = 0.0;
    double mock_util = 1.0;
    test_adjust_platform(policy, mock_active, mock_util, M_FREQ_EFFICIENT);
}

TEST_F(GPUActivityAgentTest, adjust_platform_signal_out_of_bounds_high)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = 987654321;
    double mock_util = 1.0;
    test_adjust_platform(policy, mock_active, mock_util, M_FREQ_MAX);
}

TEST_F(GPUActivityAgentTest, adjust_platform_signal_out_of_bounds_low)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    double mock_active = -12345;
    double mock_util = 1.0;
    test_adjust_platform(policy, mock_active, mock_util, M_FREQ_EFFICIENT);
}

TEST_F(GPUActivityAgentTest, adjust_platform_long_idle)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    set_up_val_policy_expectations();
    EXPECT_NO_THROW(m_agent->validate_policy(policy));

    double mock_active = 0.0;
    double mock_util = 0.0;

    // We should see one write to efficient freq, subsequent writes are masked
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MIN_IDX, M_FREQ_EFFICIENT)).Times(M_NUM_GPU_CHIP);
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MAX_IDX, M_FREQ_EFFICIENT)).Times(M_NUM_GPU_CHIP);

    // We should see one write to min freq after long idle
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MIN_IDX, M_FREQ_MIN)).Times(M_NUM_GPU_CHIP);
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MAX_IDX, M_FREQ_MIN)).Times(M_NUM_GPU_CHIP);

    for (int i = 0; i < 10; i++) {
        //Sample
        std::vector<double> tmp;
        EXPECT_CALL(*m_platform_io, sample(GPU_CORE_ACTIVITY_IDX))
                 .WillRepeatedly(Return(mock_active));
        EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                 .WillRepeatedly(Return(mock_util));
        EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                 .WillRepeatedly(Return(123456789 + i));
        EXPECT_CALL(*m_platform_io, sample(TIME_IDX))
                 .Times(1);
        m_agent->sample_platform(tmp);

        //Adjust
        m_agent->adjust_platform(policy);

        if(i == 0 || i == 9) {
            //Check a frequency decision resulted in write batch being true
            EXPECT_TRUE(m_agent->do_write_batch());
        }
        else {
            //Check a frequency decision resulted in write batch being false
            EXPECT_FALSE(m_agent->do_write_batch());
        }
    }

    std::vector<std::pair<std::string, std::string> > expected_header = { {"Agent Domain", "gpu_chip"},
                                                                          {"GPU Frequency Requests", "2.000000"},
                                                                          {"GPU Clipped Frequency Requests", "0.000000"},
                                                                          {"Resolved Max Frequency", std::to_string(M_FREQ_MAX)},
                                                                          {"Resolved Efficient Frequency", std::to_string(M_FREQ_EFFICIENT)},
                                                                          {"Resolved Frequency Range", std::to_string(M_FREQ_MAX - M_FREQ_EFFICIENT)},
                                                                          {"GPU 0 Active Region Energy", "0.000000"},
                                                                          {"GPU 0 Active Region Time", "0.000000"},
                                                                          {"GPU 0 On Energy", "0"},
                                                                          {"GPU 0 On Time", "0.000000"},
                                                                          {"GPU Chip 0 Idle Agent Actions", "1"}};
    std::vector<std::pair<std::string, std::string> > report_header = m_agent->report_host();

    EXPECT_EQ(expected_header.size(), report_header.size());
    for (int i = 0; i < expected_header.size(); ++i) {
        EXPECT_EQ(expected_header.at(i).first, report_header.at(i).first);
        if (expected_header.at(i).first != "Agent Domain") {
            EXPECT_EQ(std::stod(expected_header.at(i).second), std::stod(report_header.at(i).second));
        };
    }
}

// this tests a 'full on' waveform
// waveform: ‾‾‾‾‾‾‾‾
TEST_F(GPUActivityAgentTest, header_check_full_util)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    set_up_val_policy_expectations();
    EXPECT_NO_THROW(m_agent->validate_policy(policy));

    double mock_active = 0.12345;
    double mock_util = 1.0;

    double expected_freq = M_FREQ_EFFICIENT +
            (M_FREQ_MAX - M_FREQ_EFFICIENT) * mock_active;

    //Check frequency
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MIN_IDX, expected_freq)).Times(M_NUM_GPU_CHIP);
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MAX_IDX, expected_freq)).Times(M_NUM_GPU_CHIP);

    // waveform: ‾‾‾‾‾‾‾‾
    for (int i = 0; i < 10; i++) {
        //Sample
        std::vector<double> tmp;
        EXPECT_CALL(*m_platform_io, sample(GPU_CORE_ACTIVITY_IDX))
                 .WillRepeatedly(Return(mock_active));
        EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                 .WillRepeatedly(Return(mock_util));
        EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                 .WillRepeatedly(Return(123456789 + i));
        EXPECT_CALL(*m_platform_io, sample(TIME_IDX))
                 .WillRepeatedly(Return(21 + i*2));
        m_agent->sample_platform(tmp);

        //Adjust
        m_agent->adjust_platform(policy);
    }

    std::vector<std::pair<std::string, std::string> > expected_header = { {"Agent Domain", "gpu_chip"},
                                                                          {"GPU Frequency Requests", "1"},
                                                                          {"GPU Clipped Frequency Requests", "0"},
                                                                          {"Resolved Max Frequency", std::to_string(M_FREQ_MAX)},
                                                                          {"Resolved Efficient Frequency", std::to_string(M_FREQ_EFFICIENT)},
                                                                          {"Resolved Frequency Range", std::to_string(M_FREQ_MAX - M_FREQ_EFFICIENT)},
                                                                          {"GPU 0 Active Region Energy", "9"},
                                                                          {"GPU 0 Active Region Time", "18"},
                                                                          {"GPU 0 On Energy", "9"},
                                                                          {"GPU 0 On Time", "18"},
                                                                          {"GPU Chip 0 Idle Agent Actions", "0"}};
    std::vector<std::pair<std::string, std::string> > report_header = m_agent->report_host();

    EXPECT_EQ(expected_header.size(), report_header.size());
    for (int i = 0; i < expected_header.size(); ++i) {
        EXPECT_EQ(expected_header.at(i).first, report_header.at(i).first);
        if (expected_header.at(i).first != "Agent Domain") {
            EXPECT_EQ(std::stod(expected_header.at(i).second), std::stod(report_header.at(i).second));
        };
    }
}

// this tests a 'off on off on' waveform
// waveform: _‾_‾_‾_‾_‾_
TEST_F(GPUActivityAgentTest, header_check_on_off_util)
{
    std::vector<double> policy = M_DEFAULT_POLICY;
    set_up_val_policy_expectations();
    EXPECT_NO_THROW(m_agent->validate_policy(policy));

    // waveform: _‾_‾_‾_‾_‾_
    // Five on samples
    // Seven off samples
    // Nine 'active region' samples from to last high sample
    for (int i = 0; i < 11; i++) {

        double mock_active = i % 2;
        double mock_util = i % 2;

        double expected_freq = M_FREQ_EFFICIENT +
                               (M_FREQ_MAX - M_FREQ_EFFICIENT) * mock_active;

        //Check frequency
        EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MIN_IDX, expected_freq)).Times(M_NUM_GPU_CHIP);
        EXPECT_CALL(*m_platform_io, adjust(GPU_FREQUENCY_CONTROL_MAX_IDX, expected_freq)).Times(M_NUM_GPU_CHIP);

        //Sample
        std::vector<double> tmp;
        EXPECT_CALL(*m_platform_io, sample(GPU_CORE_ACTIVITY_IDX))
                 .WillRepeatedly(Return(mock_active));
        EXPECT_CALL(*m_platform_io, sample(GPU_UTILIZATION_IDX))
                 .WillRepeatedly(Return(mock_util));
        EXPECT_CALL(*m_platform_io, sample(GPU_ENERGY_IDX))
                 .WillRepeatedly(Return(123456789 + i));
        EXPECT_CALL(*m_platform_io, sample(TIME_IDX))
                 .WillRepeatedly(Return(21 + i*2));
        m_agent->sample_platform(tmp);

        //Adjust
        m_agent->adjust_platform(policy);
    }

    std::vector<std::pair<std::string, std::string> > expected_header = { {"Agent Domain", "gpu_chip"},
                                                                          {"GPU Frequency Requests", "11"},
                                                                          {"GPU Clipped Frequency Requests", "0"},
                                                                          {"Resolved Max Frequency", std::to_string(M_FREQ_MAX)},
                                                                          {"Resolved Efficient Frequency", std::to_string(M_FREQ_EFFICIENT)},
                                                                          {"Resolved Frequency Range", std::to_string(M_FREQ_MAX - M_FREQ_EFFICIENT)},
                                                                          {"GPU 0 Active Region Energy", "9"},
                                                                          {"GPU 0 Active Region Time", "18"},
                                                                          {"GPU 0 On Energy", "5"},
                                                                          {"GPU 0 On Time", "10"},
                                                                          {"GPU Chip 0 Idle Agent Actions", "0"}};
    std::vector<std::pair<std::string, std::string> > report_header = m_agent->report_host();

    for (const auto&[first, second] : report_header) {
        std::cout << "\t"  << first << ": " << second << std::endl;
    }

    EXPECT_EQ(expected_header.size(), report_header.size());
    for (int i = 0; i < expected_header.size(); ++i) {
        EXPECT_EQ(expected_header.at(i).first, report_header.at(i).first);
        if (expected_header.at(i).first != "Agent Domain") {
            EXPECT_EQ(std::stod(expected_header.at(i).second), std::stod(report_header.at(i).second));
        };
    }
}

TEST_F(GPUActivityAgentTest, invalid_fe)
{
    ON_CALL(*m_platform_io, read_signal("CONST_CONFIG::GPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(1e99));

    GEOPM_EXPECT_THROW_MESSAGE(m_agent->init(0, {}, false), GEOPM_ERROR_INVALID,
                                "(): GPU efficient frequency out of range: ");

    ON_CALL(*m_platform_io, read_signal("CONST_CONFIG::GPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY", GEOPM_DOMAIN_BOARD, 0))
            .WillByDefault(Return(-1));

    GEOPM_EXPECT_THROW_MESSAGE(m_agent->init(0, {}, false), GEOPM_ERROR_INVALID,
                                "(): GPU efficient frequency out of range: ");
}

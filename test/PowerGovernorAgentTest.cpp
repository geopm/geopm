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

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Exception.hpp"
#include "PowerGovernorAgent.hpp"
#include "MockPowerGovernor.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "Helper.hpp"

using geopm::PowerGovernorAgent;
using geopm::PlatformTopo;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SaveArg;
using ::testing::SetArgReferee;

bool is_format_double(std::function<std::string(double)> func);

class PowerGovernorAgentTest : public ::testing::Test
{
    protected:
        enum {
            M_SIGNAL_POWER_PACKAGE,
        };
        void SetUp(void);
        void set_up_leaf(void);
        void check_result(const std::vector<double> &expected,
                          const std::vector<double> &result);

        std::unique_ptr<MockPowerGovernor> m_power_gov;
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        double m_val_cache = 0.0;
        double m_energy_package = 0.0;
        double m_power_min = 50;
        double m_power_max = 300;
        double m_power_tdp = 250;
        std::vector<int> m_fan_in;
        int m_num_package = 2;
        int m_min_num_converged = 15;  // this is hard coded in the agent; determines how many times we need to sample
        int m_ascend_period = 10;      // also hardcoded; determines how many times we need to ascend
        int m_samples_per_control = 10;
        int m_updates_per_sample = 5;
        std::unique_ptr<PowerGovernorAgent> m_agent;
};

void PowerGovernorAgentTest::SetUp(void)
{
    // Warning: if ENERGY_PACKAGE does not return updated values,
    // PowerGovernorAgent::wait() will loop forever.
    m_energy_package = 555.5;
    ON_CALL(m_platform_io, read_signal("ENERGY_PACKAGE", _, _))
        .WillByDefault(testing::InvokeWithoutArgs([this] {
                    m_energy_package += 10.0; return m_energy_package;
                }));

    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_MIN", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(m_power_min));
    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_MAX", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(m_power_max));
    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_TDP", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(m_power_tdp));

    m_fan_in = {2, 2};
    m_power_gov = std::unique_ptr<MockPowerGovernor>(new MockPowerGovernor());
}

void PowerGovernorAgentTest::set_up_leaf(void)
{
    EXPECT_CALL(m_platform_io, control_domain_type("POWER_PACKAGE_LIMIT"))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(GEOPM_DOMAIN_PACKAGE));
    EXPECT_CALL(m_platform_io, push_signal("POWER_PACKAGE", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(M_SIGNAL_POWER_PACKAGE));
    EXPECT_CALL(*m_power_gov, init_platform_io());
    EXPECT_CALL(*m_power_gov, sample_platform())
        .Times(AtLeast(0));
    EXPECT_CALL(*m_power_gov, adjust_platform(_, _))
        .Times(AtLeast(0))
        .WillRepeatedly(DoAll(SaveArg<0>(&m_val_cache), SetArgReferee<1>(m_val_cache)));
    EXPECT_CALL(*m_power_gov, adjust_platform(_, _))
        .Times(AtLeast(0));
    EXPECT_CALL(*m_power_gov, do_write_batch())
        .Times(AtLeast(0))
        .WillRepeatedly(Return(true));
    m_agent = geopm::make_unique<PowerGovernorAgent>(m_platform_io, m_platform_topo, std::move(m_power_gov));
}

// check if containers are equal, including NAN
void PowerGovernorAgentTest::check_result(const std::vector<double> &expected,
                                          const std::vector<double> &result)
{
    ASSERT_EQ(expected.size(), result.size());
    for (size_t ii = 0; ii < expected.size(); ++ii) {
        if (std::isnan(expected[ii])) {
            EXPECT_TRUE(std::isnan(result[ii]));
        }
        else {
            EXPECT_EQ(expected[ii], result[ii]);
        }
    }
}

TEST_F(PowerGovernorAgentTest, wait)
{
    m_agent = geopm::make_unique<PowerGovernorAgent>(m_platform_io, m_platform_topo, nullptr);
    m_agent->init(1, m_fan_in, false);
    geopm_time_s start_time, end_time;
    m_agent->wait();
    geopm_time(&start_time);
    m_agent->wait();
    geopm_time(&end_time);
    EXPECT_NEAR(0.005, geopm_time_diff(&start_time, &end_time), 0.0001);
}

TEST_F(PowerGovernorAgentTest, sample_platform)
{
    set_up_leaf();
    m_agent->init(0, m_fan_in, false);
    // initial power budget
    m_agent->adjust_platform({100});
    EXPECT_TRUE(m_agent->do_write_batch());

    EXPECT_CALL(m_platform_io, sample(M_SIGNAL_POWER_PACKAGE)).Times(m_min_num_converged + 1)
        .WillRepeatedly(Return(50.5));
    std::vector<double> out_sample {NAN, NAN, NAN};
    std::vector<double> expected {NAN, NAN, NAN};

    for (int i = 0; i < m_min_num_converged; ++i) {
        m_agent->sample_platform(out_sample);
        check_result(expected, out_sample);
    }

    expected = {50.5, true, 0.0};
    m_agent->sample_platform(out_sample);
    check_result(expected, out_sample);
}

TEST_F(PowerGovernorAgentTest, adjust_platform)
{
    set_up_leaf();
    m_agent->init(0, m_fan_in, false);

    double power_budget = 123;
    std::vector<double> policy = {power_budget};

    EXPECT_CALL(m_platform_io, sample(M_SIGNAL_POWER_PACKAGE)).Times(1)
        .WillRepeatedly(Return(5.5));
    std::vector<double> out_sample {NAN, NAN, NAN};
    m_agent->sample_platform(out_sample);

    // adjust will be called once within m_samples_per_control control loops
    {
        for (int i = 0; i < m_samples_per_control; ++i) {
            m_agent->adjust_platform(policy);
            EXPECT_TRUE(m_agent->do_write_batch());
        }
    }

    // adjust will be called once for each new budget
    {
        for (int i = 0; i < m_samples_per_control; ++i) {
            power_budget += 1;
            policy = {power_budget};
            m_agent->adjust_platform(policy);
            EXPECT_TRUE(m_agent->do_write_batch());
        }
    }
}

TEST_F(PowerGovernorAgentTest, aggregate_sample)
{
    m_agent = geopm::make_unique<PowerGovernorAgent>(m_platform_io, m_platform_topo, nullptr);
    m_agent->init(1, m_fan_in, false);

    std::vector<std::vector<double> > in_sample {{2.2, false, 1.0}, {3.3, true, 2.0}};
    std::vector<double> out_sample {NAN, NAN, NAN};
    // always false if not converged
    for (int i = 0; i < m_ascend_period * 2; ++i) {
        m_agent->aggregate_sample(in_sample, out_sample);
        EXPECT_FALSE(m_agent->do_send_sample());
    }

    // once per m_ascend_period if converged
    in_sample = {{2.3, true}, {3.4, true}};
    // average of power samples
    std::vector<double> expected {(2.3 + 3.4)/2.0, true, 1.5};
    m_agent->aggregate_sample(in_sample, out_sample);
    EXPECT_TRUE(m_agent->do_send_sample());
    check_result(expected, out_sample);
    for (int i = 1; i < m_ascend_period; ++i) {
        m_agent->aggregate_sample(in_sample, out_sample);
        EXPECT_FALSE(m_agent->do_send_sample());
    }
    m_agent->aggregate_sample(in_sample, out_sample);
    EXPECT_TRUE(m_agent->do_send_sample());
}

TEST_F(PowerGovernorAgentTest, split_policy)
{
    m_agent = geopm::make_unique<PowerGovernorAgent>(m_platform_io, m_platform_topo, nullptr);
    m_agent->init(1, m_fan_in, false);

    std::vector<double> policy_in;
    std::vector<std::vector<double> > policy_out {{NAN}, {NAN}};

    // invalid budget
    EXPECT_THROW(m_agent->split_policy({10}, policy_out), geopm::Exception);

    // all children get same budget
    policy_in = {100};
    m_agent->split_policy(policy_in, policy_out);
    EXPECT_TRUE(m_agent->do_send_policy());
    std::vector<std::vector<double> > expected {{100}, {100}};
    for (int child = 0; child < m_fan_in[1]; ++child) {
        check_result(expected[child], policy_out[child]);
    }

    // budget stays the same
    for (int i = 0; i < 50; ++i) {
        m_agent->split_policy(policy_in, policy_out);
        EXPECT_FALSE(m_agent->do_send_policy());
    }

    // updated budget
    policy_in = {150};
    m_agent->split_policy(policy_in, policy_out);
    EXPECT_TRUE(m_agent->do_send_policy());
    expected = {{150}, {150}};
    for (int child = 0; child < m_fan_in[1]; ++child) {
        check_result(expected[child], policy_out[child]);
    }
}

TEST_F(PowerGovernorAgentTest, enforce_policy)
{
    const double limit = 100;
    const std::vector<double> policy{limit};
    const std::vector<double> bad_policy{100, 200, 300};

    EXPECT_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillOnce(Return(m_num_package));
    EXPECT_CALL(m_platform_io, control_domain_type("POWER_PACKAGE_LIMIT"))
        .WillOnce(Return(GEOPM_DOMAIN_PACKAGE));
    EXPECT_CALL(m_platform_io, write_control("POWER_PACKAGE_LIMIT", GEOPM_DOMAIN_BOARD,
                                             0, limit/m_num_package));

    m_agent = geopm::make_unique<PowerGovernorAgent>(m_platform_io, m_platform_topo, nullptr);
    m_agent->enforce_policy(policy);

    EXPECT_THROW(m_agent->enforce_policy(bad_policy), geopm::Exception);
}

TEST_F(PowerGovernorAgentTest, trace)
{
    m_agent = geopm::make_unique<PowerGovernorAgent>(m_platform_io, m_platform_topo, nullptr);
    std::vector<std::string> expect_names{"POWER_BUDGET"};
    EXPECT_EQ(expect_names, m_agent->trace_names());
    EXPECT_TRUE(is_format_double(m_agent->trace_formats().at(0)));
}

TEST_F(PowerGovernorAgentTest, validate_policy)
{
    m_agent = geopm::make_unique<PowerGovernorAgent>(m_platform_io, m_platform_topo, nullptr);

    std::vector<double> policy;

    // valid policy unchanged
    policy = {100};
    m_agent->validate_policy(policy);
    EXPECT_EQ(100, policy[0]);

    // NAN becomes default
    policy = {NAN};
    m_agent->validate_policy(policy);
    EXPECT_EQ(m_power_tdp, policy[0]);

    // clamp to min
    policy = {m_power_min - 1};
    m_agent->validate_policy(policy);
    EXPECT_EQ(m_power_min, policy[0]);

    // clamp to max
    policy = {m_power_max + 1};
    m_agent->validate_policy(policy);
    EXPECT_EQ(m_power_max, policy[0]);
}

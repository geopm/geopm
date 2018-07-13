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

#include <vector>
#include <memory>
#include <sstream>
#include <list>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Kontroller.hpp"
#include "PowerGovernorAgent.hpp"

#include "MockPlatformTopo.hpp"
#include "MockPlatformIO.hpp"
#include "MockComm.hpp"
#include "MockApplicationIO.hpp"
#include "MockManagerIOSampler.hpp"
#include "MockAgent.hpp"
#include "MockTreeComm.hpp"
#include "MockReporter.hpp"
#include "MockTracer.hpp"
#include "Helper.hpp"

using geopm::Kontroller;
using geopm::IPlatformIO;
using geopm::IPlatformTopo;
using geopm::ApplicationIO;
using geopm::Agent;
using geopm::PowerGovernorAgent;
using testing::NiceMock;
using testing::_;
using testing::Return;
using testing::AtLeast;
using testing::Invoke;

class KontrollerGovernorTestMockPlatformIO : public MockPlatformIO
{
    public:
        KontrollerGovernorTestMockPlatformIO()
        {
            // todo: wrong function for some signals
            ON_CALL(*this, agg_function(_))
                .WillByDefault(Return(IPlatformIO::agg_sum));

            // any other "unsupported" signals
            ON_CALL(*this, push_signal(_, _, _))
                .WillByDefault(Return(-1));
            ON_CALL(*this, sample(-1))
                .WillByDefault(Return(NAN));
        }
        int add_supported_signal(const IPlatformIO::m_request_s &signal, double default_value)
        {
            int result = m_index;
            ON_CALL(*this, push_signal(signal.name, signal.domain_type, signal.domain_idx))
                .WillByDefault(Return(m_index));
            ON_CALL(*this, sample(m_index))
                .WillByDefault(Return(default_value));
            ON_CALL(*this, read_signal(signal.name, signal.domain_type, signal.domain_idx))
                .WillByDefault(Return(default_value));
            ++m_index;
            return result;

        }
        int add_supported_control(const IPlatformIO::m_request_s &control)
        {
            int result = m_last_adjust.size();
            m_last_adjust.push_back(NAN);
            ON_CALL(*this, push_control(control.name, control.domain_type, control.domain_idx))
                .WillByDefault(Return(result));
            ON_CALL(*this, control_domain_type(control.name))
                .WillByDefault(Return(control.domain_type));
            ON_CALL(*this, adjust(result, _))
                .WillByDefault(Invoke(
                    [this] (int control_idx, double value)
                    {
                        if (control_idx < 0 || (size_t)control_idx >= m_last_adjust.size()) {
                            std::cerr << "control_idx out of bounds." << std::endl;
                        }
                        else {
                            m_last_adjust[control_idx] = value;
                        }
                    }));
            return result;
        }
        double get_last_adjusted_value(int control_idx)
        {
            if (control_idx < 0 || (size_t)control_idx >= m_last_adjust.size()) {
                throw std::runtime_error("control_idx out of bounds.");
            }
            return m_last_adjust[control_idx];
        }
        void add_varying_signal(const IPlatformIO::m_request_s &signal, const std::vector<double> &values)
        {
            int signal_index = m_signal_value.size();
            m_signal_value.push_back(values);
            m_next_value_idx.push_back(0ull);
            ON_CALL(*this, push_signal(signal.name, signal.domain_type, signal.domain_idx))
                .WillByDefault(Return(signal_index));
            ON_CALL(*this, sample(signal_index))
                .WillByDefault(Invoke(
                    [this] (int index) -> double
                    {
                        double result = NAN;
                        if (index < 0 || (size_t)index >= m_next_value_idx.size()) {
                            std::cerr << "invalid signal index" << std::endl;
                        }
                        else if (m_next_value_idx[index] >= m_signal_value[index].size()) {
                            std::cerr << "varying signal ran out of values; too many calls to sample()" << std::endl;
                        }
                        else {
                            result = m_signal_value[index][m_next_value_idx[index]];
                            ++m_next_value_idx[index];
                        }
                        return result;
                    }));
        }

    private:
        /// Unique index used for supported signals with single default value
        int m_index = 0;
        /// Used to spy on value adjusted for supported controls
        std::vector<double> m_last_adjust;
        /// Values to use for invocations of sample() for each signal
        std::vector<std::vector<double> > m_signal_value;
        /// Index into the m_signal_value vector for each signal
        std::vector<size_t> m_next_value_idx;
};

class KontrollerPowerGovernorTest : public ::testing::Test
{
    protected:
        void SetUp();

        std::string m_agent_name = "power_governor";
        int m_num_send_up = PowerGovernorAgent::sample_names().size();
        int m_num_send_down = PowerGovernorAgent::policy_names().size();
        std::shared_ptr<MockComm> m_comm;
        NiceMock<KontrollerGovernorTestMockPlatformIO> m_platform_io;
        NiceMock<MockPlatformTopo> m_platform_topo;
        std::shared_ptr<MockApplicationIO> m_application_io;
        MockTreeComm *m_tree_comm;
        MockReporter *m_reporter;
        MockTracer *m_tracer;
        std::vector<PowerGovernorAgent*> m_level_agent;
        std::vector<std::unique_ptr<Agent> > m_agents;
        MockManagerIOSampler *m_manager_io;

        int m_num_step = 500;
        std::list<geopm_region_info_s> m_region_info;
        std::vector<std::pair<std::string, std::string> > m_agent_report;
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > m_region_names;
        double m_energy_package = 0.0;
        double m_power_budget = 200;
        double m_power_min = 50;
        double m_power_max = 300;
        int m_power_control_idx = -1;

        int m_samples_per_control = 10;  // should match agent
};

void KontrollerPowerGovernorTest::SetUp()
{
    m_platform_io.add_supported_signal({"POWER_PACKAGE_MIN", IPlatformTopo::M_DOMAIN_PACKAGE, 0}, m_power_min);
    m_platform_io.add_supported_signal({"POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0}, m_power_max);

    m_power_control_idx = m_platform_io.add_supported_control({"POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0});

    // Warning: if ENERGY_PACKAGE does not return updated values,
    // PowerGovernorAgent::wait() will loop forever.
    m_energy_package = 555.5;
    ON_CALL(m_platform_io, read_signal("ENERGY_PACKAGE", _, _))
        .WillByDefault(testing::InvokeWithoutArgs([this] {
                    m_energy_package += 10.0; return m_energy_package;
                }));

    m_comm = std::make_shared<MockComm>();
    m_application_io = std::make_shared<NiceMock<MockApplicationIO> >();
    m_tree_comm = new NiceMock<MockTreeComm>();
    m_manager_io = new NiceMock<MockManagerIOSampler>();
    m_reporter = new NiceMock<MockReporter>();
    m_tracer = new NiceMock<MockTracer>();

    ON_CALL(*m_application_io, region_info()).WillByDefault(Return(m_region_info));
    std::vector<double> manager_sample = {m_power_budget};
    ASSERT_EQ(m_num_send_down, (int)manager_sample.size());
    ON_CALL(*m_manager_io, sample()).WillByDefault(Return(manager_sample));

    // todo: not quite right but works for now
    ON_CALL(m_platform_topo, num_domain(_)).WillByDefault(Return(1));
}

TEST_F(KontrollerPowerGovernorTest, single_node)
{
    int num_level_ctl = 0;
    int root_level = 0;
    EXPECT_CALL(*m_tree_comm, num_level_controlled())
        .WillOnce(Return(num_level_ctl));
    EXPECT_CALL(*m_tree_comm, root_level())
        .WillOnce(Return(root_level));

    std::vector<double> power_package;
    std::vector<double> power_dram;
    for (int step = 0; step < m_num_step; ++step) {
        // 1 extra sample to handle power correctly
        for (int sample = 0; sample < m_samples_per_control + 1; ++sample) {
            power_package.push_back(m_power_budget / 2 + 0.01*step + 0.001*sample);
            power_dram.push_back(12.0 + 0.005 * step);
        }
    }
    m_platform_io.add_varying_signal({"POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0},
                                     power_package);
    m_platform_io.add_varying_signal({"POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0},
                                     power_dram);

    auto agent = new PowerGovernorAgent(m_platform_io, m_platform_topo);
    std::vector<int> fan_in = {0};
    agent->init(0, fan_in, true);
    m_agents.emplace_back(agent);

    Kontroller kontroller(m_comm, m_platform_io,
                          m_agent_name, m_num_send_down, m_num_send_up,
                          std::unique_ptr<MockTreeComm>(m_tree_comm),
                          m_application_io,
                          std::unique_ptr<MockReporter>(m_reporter),
                          std::unique_ptr<MockTracer>(m_tracer),
                          std::move(m_agents),
                          std::unique_ptr<MockManagerIOSampler>(m_manager_io));
    kontroller.setup_trace();

    for (int step = 0; step < m_num_step; ++step) {
        for (int sample = 0; sample < m_samples_per_control + 1; ++sample) {
            kontroller.step();
        }
        double expected_budget = m_power_budget - 11.0;
        EXPECT_GT(expected_budget, m_platform_io.get_last_adjusted_value(m_power_control_idx));
        EXPECT_LT(m_power_min, m_platform_io.get_last_adjusted_value(m_power_control_idx));
    }

    // single node Kontroller should not send anything via TreeComm
    EXPECT_EQ(0, m_tree_comm->num_send());
    EXPECT_EQ(0, m_tree_comm->num_recv());
}

// controller with only leaf responsibilities
TEST_F(KontrollerPowerGovernorTest, two_level_controller_1)
{
    int num_level_ctl = 0;
    int root_level = 2;
    EXPECT_CALL(*m_tree_comm, num_level_controlled())
        .WillOnce(Return(num_level_ctl));
    EXPECT_CALL(*m_tree_comm, root_level())
        .WillOnce(Return(root_level));

    std::vector<double> power_package;
    std::vector<double> power_dram;
    for (int step = 0; step < m_num_step; ++step) {
        // 1 extra sample to handle power correctly
        for (int sample = 0; sample < m_samples_per_control + 1; ++sample) {
            power_package.push_back(m_power_budget / 2 + 0.01*step + 0.001*sample);
            power_dram.push_back(12.0 + 0.005 * step);
        }
    }
    m_platform_io.add_varying_signal({"POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0},
                                     power_package);
    m_platform_io.add_varying_signal({"POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0},
                                     power_dram);

    auto agent = new PowerGovernorAgent(m_platform_io, m_platform_topo);
    std::vector<int> fan_in = {0};
    agent->init(0, fan_in, true);
    m_agents.emplace_back(agent);

    Kontroller kontroller(m_comm, m_platform_io,
                          m_agent_name, m_num_send_down, m_num_send_up,
                          std::unique_ptr<MockTreeComm>(m_tree_comm),
                          m_application_io,
                          std::unique_ptr<MockReporter>(m_reporter),
                          std::unique_ptr<MockTracer>(m_tracer),
                          std::move(m_agents),
                          std::unique_ptr<MockManagerIOSampler>(m_manager_io));
    kontroller.setup_trace();

    // mock parent sending to this child
    std::vector<std::vector<double> > policy = {{m_power_budget - 5.0}, {m_power_budget + 5.0}};
    m_tree_comm->send_down(num_level_ctl, policy);

    // should not interact with manager io
    EXPECT_CALL(*m_manager_io, sample()).Times(0);


    for (int step = 0; step < m_num_step; ++step) {
        for (int sample = 0; sample < m_samples_per_control + 1; ++sample) {
            kontroller.step();
        }
        // this is first child, so use first budget from parent
        double expected_budget = m_power_budget - 11.0;
        EXPECT_GT(expected_budget, m_platform_io.get_last_adjusted_value(m_power_control_idx));
        EXPECT_LT(m_power_min, m_platform_io.get_last_adjusted_value(m_power_control_idx));
    }

    EXPECT_NE(0, m_tree_comm->num_send());
    EXPECT_NE(0, m_tree_comm->num_recv());
}

// controller with leaf and tree responsibilities, but not at the root
TEST_F(KontrollerPowerGovernorTest, two_level_controller_2)
{
    int num_level_ctl = 1;
    int root_level = 2;
    std::vector<int> fan_out = {2, 2};
    ASSERT_EQ(root_level, (int)fan_out.size());

    EXPECT_CALL(*m_tree_comm, num_level_controlled())
        .WillOnce(Return(num_level_ctl));
    EXPECT_CALL(*m_tree_comm, root_level())
        .WillOnce(Return(root_level));
    for (int level = 0; level < num_level_ctl; ++level) {
        EXPECT_CALL(*m_tree_comm, level_size(level)).WillOnce(Return(fan_out[level]));
    }

    std::vector<double> power_package;
    std::vector<double> power_dram;
    double dram_power = 12.0;
    for (int step = 0; step < m_num_step; ++step) {
        // 1 extra sample to handle power correctly
        for (int sample = 0; sample < m_samples_per_control + 1; ++sample) {
            power_package.push_back(m_power_budget / 2 + 0.01*step + 0.001*sample);
            power_dram.push_back(dram_power + 0.005 * step);
        }
    }
    m_platform_io.add_varying_signal({"POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0},
                                     power_package);
    m_platform_io.add_varying_signal({"POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0},
                                     power_dram);


    for (int level = 0; level < num_level_ctl + 1; ++level) {
        auto tmp = new PowerGovernorAgent(m_platform_io, m_platform_topo);
        tmp->init(level, fan_out, true);
        m_level_agent.push_back(tmp);

        m_agents.emplace_back(m_level_agent[level]);
    }
    ASSERT_EQ(2u, m_level_agent.size());

    // should not interact with manager io
    EXPECT_CALL(*m_manager_io, sample()).Times(0);


    Kontroller kontroller(m_comm, m_platform_io,
                          m_agent_name, m_num_send_down, m_num_send_up,
                          std::unique_ptr<MockTreeComm>(m_tree_comm),
                          m_application_io,
                          std::unique_ptr<MockReporter>(m_reporter),
                          std::unique_ptr<MockTracer>(m_tracer),
                          std::move(m_agents),
                          std::unique_ptr<MockManagerIOSampler>(m_manager_io));
    kontroller.setup_trace();

    // mock parent sending to this child
    double parent_policy_offset = 6.0;
    std::vector<std::vector<double> > policy = {{m_power_budget - parent_policy_offset},
                                                {m_power_budget + parent_policy_offset}};
    m_tree_comm->send_down(num_level_ctl, policy);


    for (int step = 0; step < m_num_step; ++step) {
        // mock other child
        std::vector<double> sample = {100.0 + (step *0.0001), 67.0, true};
        m_tree_comm->send_up_mock_child(0, 1, sample);
        for (int sample = 0; sample < m_samples_per_control + 1; ++sample) {
            kontroller.step();
        }
        double expected_budget = m_power_budget - dram_power - parent_policy_offset + 1.0;
        EXPECT_GT(expected_budget, m_platform_io.get_last_adjusted_value(m_power_control_idx));
        EXPECT_LT(m_power_min, m_platform_io.get_last_adjusted_value(m_power_control_idx));
    }

    EXPECT_NE(0, m_tree_comm->num_send());
    EXPECT_NE(0, m_tree_comm->num_recv());
}

// controller with responsibilities at all levels of the tree
TEST_F(KontrollerPowerGovernorTest, two_level_controller_0)
{
    int num_level_ctl = 2;
    int root_level = 2;
    std::vector<int> fan_out = {2, 2};
    ASSERT_EQ(root_level, (int)fan_out.size());

    EXPECT_CALL(*m_tree_comm, num_level_controlled())
        .WillOnce(Return(num_level_ctl));
    EXPECT_CALL(*m_tree_comm, root_level())
        .WillOnce(Return(root_level));
    for (int level = 0; level < num_level_ctl; ++level) {
        EXPECT_CALL(*m_tree_comm, level_size(level)).WillOnce(Return(fan_out[level]));
    }

    std::vector<double> power_package;
    std::vector<double> power_dram;
    for (int step = 0; step < m_num_step; ++step) {
        // 1 extra sample to handle power correctly
        for (int sample = 0; sample < m_samples_per_control + 1; ++sample) {
            power_package.push_back(m_power_budget / 2 + 0.01*step + 0.001*sample);
            power_dram.push_back(12.0 + 0.005 * step);
        }
    }
    m_platform_io.add_varying_signal({"POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0},
                                     power_package);
    m_platform_io.add_varying_signal({"POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0},
                                     power_dram);

    for (int level = 0; level < num_level_ctl + 1; ++level) {
        auto tmp = new PowerGovernorAgent(m_platform_io, m_platform_topo);
        tmp->init(level, fan_out, true);
        m_level_agent.push_back(tmp);

        m_agents.emplace_back(m_level_agent[level]);
    }
    ASSERT_EQ(3u, m_level_agent.size());

    Kontroller kontroller(m_comm, m_platform_io,
                          m_agent_name, m_num_send_down, m_num_send_up,
                          std::unique_ptr<MockTreeComm>(m_tree_comm),
                          m_application_io,
                          std::unique_ptr<MockReporter>(m_reporter),
                          std::unique_ptr<MockTracer>(m_tracer),
                          std::move(m_agents),
                          std::unique_ptr<MockManagerIOSampler>(m_manager_io));
    kontroller.setup_trace();

    for (int step = 0; step < m_num_step; ++step) {
        // mock other children
        std::vector<double> sample = {78.0 + step, 67.0 + step, true};
        m_tree_comm->send_up_mock_child(0, 1, sample);
        sample = {77.0 + step, 66.0 + step, true};
        m_tree_comm->send_up_mock_child(1, 1, sample);

        for (int sample = 0; sample < m_samples_per_control + 1; ++sample) {
            kontroller.step();
        }
        double expected_budget = m_power_budget - 11.0;
        EXPECT_GT(expected_budget, m_platform_io.get_last_adjusted_value(m_power_control_idx));
        EXPECT_LT(m_power_min, m_platform_io.get_last_adjusted_value(m_power_control_idx));
    }

    EXPECT_NE(0, m_tree_comm->num_send());
    EXPECT_NE(0, m_tree_comm->num_recv());
}

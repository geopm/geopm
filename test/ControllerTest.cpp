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

#include <vector>
#include <memory>
#include <sstream>
#include <list>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Controller.hpp"

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
#include "Agg.hpp"

using geopm::Controller;
using geopm::IPlatformIO;
using geopm::IPlatformTopo;
using geopm::ApplicationIO;
using geopm::Agent;
using testing::NiceMock;
using testing::_;
using testing::Return;
using testing::AtLeast;
using testing::ContainerEq;

class ControllerTestMockPlatformIO : public MockPlatformIO
{
    public:
        ControllerTestMockPlatformIO()
        {
            ON_CALL(*this, agg_function(_))
                .WillByDefault(Return(geopm::Agg::sum));

            // any other "unsupported" signals
            ON_CALL(*this, push_signal(_, _, _))
                .WillByDefault(Return(-1));
            ON_CALL(*this, sample(-1))
                .WillByDefault(Return(NAN));
        }
        void add_supported_signal(IPlatformIO::m_request_s signal, double default_value)
        {
            ON_CALL(*this, push_signal(signal.name, signal.domain_type, signal.domain_idx))
                .WillByDefault(Return(m_index));
            ON_CALL(*this, sample(m_index))
                .WillByDefault(Return(default_value));
            ON_CALL(*this, read_signal(signal.name, signal.domain_type, signal.domain_idx))
                .WillByDefault(Return(default_value));
            ++m_index;
        }

    private:
        int m_index = 0;
};



class ControllerTest : public ::testing::Test
{
    protected:
        void SetUp();

        std::string m_agent_name = "temp";
        int m_num_send_up = 4;
        int m_num_send_down = 2;
        std::shared_ptr<MockComm> m_comm;
        ControllerTestMockPlatformIO m_platform_io;
        std::shared_ptr<MockApplicationIO> m_application_io;
        MockTreeComm *m_tree_comm;
        MockReporter *m_reporter;
        MockTracer *m_tracer;
        std::vector<MockAgent*> m_level_agent;
        std::vector<std::unique_ptr<Agent> > m_agents;
        MockManagerIOSampler *m_manager_io;

        int m_num_step = 3;
        std::list<geopm_region_info_s> m_region_info;
        std::vector<std::pair<std::string, std::string> > m_agent_report;
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > m_region_names;
};

void ControllerTest::SetUp()
{
    m_platform_io.add_supported_signal({"TIME", IPlatformTopo::M_DOMAIN_BOARD, 0}, 99);
    m_platform_io.add_supported_signal({"POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0}, 4545);
    m_platform_io.add_supported_signal({"FREQUENCY", IPlatformTopo::M_DOMAIN_BOARD, 0}, 333);
    m_platform_io.add_supported_signal({"REGION_PROGRESS", IPlatformTopo::M_DOMAIN_BOARD, 0}, 0.5);

    m_comm = std::make_shared<MockComm>();
    m_application_io = std::make_shared<MockApplicationIO>();
    m_tree_comm = new MockTreeComm();
    m_manager_io = new MockManagerIOSampler();
    m_reporter = new MockReporter();
    m_tracer = new MockTracer();

    // called during clean up
    EXPECT_CALL(m_platform_io, restore_control());
}

TEST_F(ControllerTest, single_node)
{
    int num_level_ctl = 0;
    int root_level = 0;
    auto agent = new MockAgent();
    m_agents.emplace_back(agent);

    // constructor
    EXPECT_CALL(*m_tree_comm, num_level_controlled())
        .WillOnce(Return(num_level_ctl));
    EXPECT_CALL(*m_tree_comm, root_level())
        .WillOnce(Return(root_level));
    Controller controller(m_comm, m_platform_io,
                          m_agent_name, m_num_send_down, m_num_send_up,
                          std::unique_ptr<MockTreeComm>(m_tree_comm),
                          m_application_io,
                          std::unique_ptr<MockReporter>(m_reporter),
                          std::unique_ptr<MockTracer>(m_tracer),
                          std::move(m_agents),
                          std::unique_ptr<MockManagerIOSampler>(m_manager_io));

    // setup trace
    std::vector<std::string> trace_names = {"COL1", "COL2"};
    EXPECT_CALL(*agent, trace_names()).WillOnce(Return(trace_names));
    EXPECT_CALL(*m_tracer, columns(_));
    controller.setup_trace();

    // step
    EXPECT_CALL(m_platform_io, read_batch()).Times(m_num_step);
    EXPECT_CALL(m_platform_io, write_batch()).Times(m_num_step);
    EXPECT_CALL(*m_application_io, update(_)).Times(m_num_step);
    EXPECT_CALL(*m_application_io, region_info()).Times(m_num_step)
        .WillRepeatedly(Return(m_region_info));
    EXPECT_CALL(*m_application_io, clear_region_info()).Times(m_num_step);
    std::vector<double> manager_sample = {8.8, 9.9};
    ASSERT_EQ(m_num_send_down, (int)manager_sample.size());
    EXPECT_CALL(*m_manager_io, sample()).Times(m_num_step)
        .WillRepeatedly(Return(manager_sample));
    EXPECT_CALL(*m_reporter, update()).Times(m_num_step);
    EXPECT_CALL(*m_tracer, update(_, _)).Times(m_num_step);
    EXPECT_CALL(*agent, trace_values(_)).Times(m_num_step);
    EXPECT_CALL(*agent, adjust_platform(_)).Times(m_num_step).WillRepeatedly(Return(true));
    EXPECT_CALL(*agent, sample_platform(_)).Times(m_num_step)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*agent, wait()).Times(m_num_step);
    // should not call ascend/descend
    EXPECT_CALL(*agent, ascend(_, _)).Times(0);
    EXPECT_CALL(*agent, descend(_, _)).Times(0);

    for (int step = 0; step < m_num_step; ++step) {
        controller.step();
    }

    // generate report and trace
    EXPECT_CALL(*agent, report_header()).WillOnce(Return(m_agent_report));
    EXPECT_CALL(*agent, report_node()).WillOnce(Return(m_agent_report));
    EXPECT_CALL(*agent, report_region()).WillOnce(Return(m_region_names));
    EXPECT_CALL(*m_reporter, generate(_, _, _, _, _, _, _));
    EXPECT_CALL(*m_tracer, flush());
    controller.generate();

    // single node Controller should not send anything via TreeComm
    EXPECT_EQ(0, m_tree_comm->num_send());
    EXPECT_EQ(0, m_tree_comm->num_recv());
}

// controller with only leaf responsibilities
TEST_F(ControllerTest, two_level_controller_1)
{
    int num_level_ctl = 0;
    int root_level = 2;
    EXPECT_CALL(*m_tree_comm, num_level_controlled())
        .WillOnce(Return(num_level_ctl));
    EXPECT_CALL(*m_tree_comm, root_level())
        .WillOnce(Return(root_level));

    auto agent = new MockAgent();
    m_agents.emplace_back(agent);

    Controller controller(m_comm, m_platform_io,
                          m_agent_name, m_num_send_down, m_num_send_up,
                          std::unique_ptr<MockTreeComm>(m_tree_comm),
                          m_application_io,
                          std::unique_ptr<MockReporter>(m_reporter),
                          std::unique_ptr<MockTracer>(m_tracer),
                          std::move(m_agents),
                          std::unique_ptr<MockManagerIOSampler>(m_manager_io));

    std::vector<std::string> trace_names = {"COL1", "COL2"};
    EXPECT_CALL(*agent, trace_names()).WillOnce(Return(trace_names));
    EXPECT_CALL(*m_tracer, columns(_));
    controller.setup_trace();

    // mock parent sending to this child
    std::vector<std::vector<double> > policy = {{1, 2}, {3, 4}};
    m_tree_comm->send_down(num_level_ctl, policy);
    m_tree_comm->reset_spy();

    // should not interact with manager io
    EXPECT_CALL(*m_manager_io, sample()).Times(0);

    EXPECT_CALL(m_platform_io, read_batch()).Times(m_num_step);
    EXPECT_CALL(m_platform_io, write_batch()).Times(m_num_step);
    EXPECT_CALL(*m_application_io, update(_)).Times(m_num_step);
    EXPECT_CALL(*m_application_io, region_info()).Times(m_num_step)
        .WillRepeatedly(Return(m_region_info));
    EXPECT_CALL(*m_application_io, clear_region_info()).Times(m_num_step);
    EXPECT_CALL(*m_reporter, update()).Times(m_num_step);
    EXPECT_CALL(*m_tracer, update(_, _)).Times(m_num_step);
    EXPECT_CALL(*agent, trace_values(_)).Times(m_num_step);
    EXPECT_CALL(*agent, adjust_platform(_)).Times(m_num_step).WillRepeatedly(Return(true));
    EXPECT_CALL(*agent, sample_platform(_)).Times(m_num_step)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*agent, wait()).Times(m_num_step);
    // should not call ascend/descend
    EXPECT_CALL(*agent, ascend(_, _)).Times(0);
    EXPECT_CALL(*agent, descend(_, _)).Times(0);

    for (int step = 0; step < m_num_step; ++step) {
        controller.step();
    }

    // only root should add header
    EXPECT_CALL(*agent, report_header()).Times(0);

    EXPECT_CALL(*agent, report_node()).WillOnce(Return(m_agent_report));
    EXPECT_CALL(*agent, report_region()).WillOnce(Return(m_region_names));
    EXPECT_CALL(*m_reporter, generate(_, _, _, _, _, _, _));
    EXPECT_CALL(*m_tracer, flush());
    controller.generate();

    std::set<int> send_down_levels {};
    std::set<int> recv_down_levels {0};
    std::set<int> send_up_levels {0};
    std::set<int> recv_up_levels {};
    EXPECT_THAT(send_down_levels, ContainerEq(m_tree_comm->levels_sent_down()));
    EXPECT_THAT(recv_down_levels, ContainerEq(m_tree_comm->levels_rcvd_down()));
    EXPECT_THAT(send_up_levels, ContainerEq(m_tree_comm->levels_sent_up()));
    EXPECT_THAT(recv_up_levels, ContainerEq(m_tree_comm->levels_rcvd_up()));
}

// controller with leaf and tree responsibilities, but not at the root
TEST_F(ControllerTest, two_level_controller_2)
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
    for (int level = 0; level < num_level_ctl + 1; ++level) {
        auto tmp = new MockAgent();
        EXPECT_CALL(*tmp, init(level, fan_out, true));
        tmp->init(level, fan_out, true);
        m_level_agent.push_back(tmp);

        m_agents.emplace_back(m_level_agent[level]);
    }
    ASSERT_EQ(2u, m_level_agent.size());

    Controller controller(m_comm, m_platform_io,
                          m_agent_name, m_num_send_down, m_num_send_up,
                          std::unique_ptr<MockTreeComm>(m_tree_comm),
                          m_application_io,
                          std::unique_ptr<MockReporter>(m_reporter),
                          std::unique_ptr<MockTracer>(m_tracer),
                          std::move(m_agents),
                          std::unique_ptr<MockManagerIOSampler>(m_manager_io));

    std::vector<std::string> trace_names = {"COL1", "COL2"};
    EXPECT_CALL(*m_level_agent[0], trace_names()).WillOnce(Return(trace_names));
    EXPECT_CALL(*m_tracer, columns(_));
    controller.setup_trace();

    // mock parent sending to this child
    std::vector<std::vector<double> > policy = {{1, 2}, {3, 4}};
    m_tree_comm->send_down(num_level_ctl, policy);
    m_tree_comm->reset_spy();

    // should not interact with manager io
    EXPECT_CALL(*m_manager_io, sample()).Times(0);

    EXPECT_CALL(m_platform_io, read_batch()).Times(m_num_step);
    EXPECT_CALL(*m_application_io, update(_)).Times(m_num_step);
    EXPECT_CALL(*m_application_io, region_info()).Times(m_num_step)
        .WillRepeatedly(Return(m_region_info));
    EXPECT_CALL(*m_application_io, clear_region_info()).Times(m_num_step);
    EXPECT_CALL(*m_reporter, update()).Times(m_num_step);
    EXPECT_CALL(*m_tracer, update(_, _)).Times(m_num_step);
    EXPECT_CALL(*m_level_agent[0], trace_values(_)).Times(m_num_step);
    EXPECT_CALL(*m_level_agent[0], sample_platform(_)).Times(m_num_step)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*m_level_agent[0], wait()).Times(m_num_step);
    // agent 0 should not call ascend/descend
    EXPECT_CALL(*m_level_agent[0], ascend(_, _)).Times(0);
    EXPECT_CALL(*m_level_agent[0], descend(_, _)).Times(0);

    EXPECT_CALL(*m_level_agent[1], descend(_, _)).Times(m_num_step)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*m_level_agent[1], ascend(_, _)).Times(m_num_step)
        .WillRepeatedly(Return(true));

    for (int step = 0; step < m_num_step; ++step) {
        controller.step();
    }

    for (const auto &agent : m_level_agent) {
        // only root should add header
        EXPECT_CALL(*agent, report_header()).Times(0);
    }
    EXPECT_CALL(*m_level_agent[0], report_node()).WillOnce(Return(m_agent_report));
    EXPECT_CALL(*m_level_agent[0], report_region()).WillOnce(Return(m_region_names));
    EXPECT_CALL(*m_reporter, generate(_, _, _, _, _, _, _));
    EXPECT_CALL(*m_tracer, flush());
    controller.generate();

    std::set<int> send_down_levels {0};
    std::set<int> recv_down_levels {1, 0};
    std::set<int> send_up_levels {0, 1};
    std::set<int> recv_up_levels {0};
    EXPECT_THAT(send_down_levels, ContainerEq(m_tree_comm->levels_sent_down()));
    EXPECT_THAT(recv_down_levels, ContainerEq(m_tree_comm->levels_rcvd_down()));
    EXPECT_THAT(send_up_levels, ContainerEq(m_tree_comm->levels_sent_up()));
    EXPECT_THAT(recv_up_levels, ContainerEq(m_tree_comm->levels_rcvd_up()));
}

// controller with responsibilities at all levels of the tree
TEST_F(ControllerTest, two_level_controller_0)
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
    for (int level = 0; level < num_level_ctl + 1; ++level) {
        auto tmp = new MockAgent();
        EXPECT_CALL(*tmp, init(level, fan_out, true));
        tmp->init(level, fan_out, true);
        m_level_agent.push_back(tmp);

        m_agents.emplace_back(m_level_agent[level]);
    }
    ASSERT_EQ(3u, m_level_agent.size());

    Controller controller(m_comm, m_platform_io,
                          m_agent_name, m_num_send_down, m_num_send_up,
                          std::unique_ptr<MockTreeComm>(m_tree_comm),
                          m_application_io,
                          std::unique_ptr<MockReporter>(m_reporter),
                          std::unique_ptr<MockTracer>(m_tracer),
                          std::move(m_agents),
                          std::unique_ptr<MockManagerIOSampler>(m_manager_io));

    std::vector<std::string> trace_names = {"COL1", "COL2"};
    EXPECT_CALL(*m_level_agent[0], trace_names()).WillOnce(Return(trace_names));
    EXPECT_CALL(*m_tracer, columns(_));
    controller.setup_trace();

    EXPECT_CALL(m_platform_io, read_batch()).Times(m_num_step);
    EXPECT_CALL(*m_application_io, update(_)).Times(m_num_step);
    EXPECT_CALL(*m_application_io, region_info()).Times(m_num_step)
        .WillRepeatedly(Return(m_region_info));
    EXPECT_CALL(*m_application_io, clear_region_info()).Times(m_num_step);
    std::vector<double> manager_sample = {8.8, 9.9};
    ASSERT_EQ(m_num_send_down, (int)manager_sample.size());
    EXPECT_CALL(*m_manager_io, sample()).Times(m_num_step)
        .WillRepeatedly(Return(manager_sample));
    EXPECT_CALL(*m_reporter, update()).Times(m_num_step);
    EXPECT_CALL(*m_tracer, update(_, _)).Times(m_num_step);
    EXPECT_CALL(*m_level_agent[0], trace_values(_)).Times(m_num_step);
    EXPECT_CALL(*m_level_agent[0], sample_platform(_)).Times(m_num_step)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*m_level_agent[0], wait()).Times(m_num_step);
    // agent 0 should not call ascend/descend
    EXPECT_CALL(*m_level_agent[0], ascend(_, _)).Times(0);
    EXPECT_CALL(*m_level_agent[0], descend(_, _)).Times(0);

    EXPECT_CALL(*m_level_agent[2], descend(_, _)).Times(m_num_step)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*m_level_agent[1], descend(_, _)).Times(m_num_step)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*m_level_agent[1], ascend(_, _)).Times(m_num_step)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*m_level_agent[2], ascend(_, _)).Times(m_num_step)
        .WillRepeatedly(Return(true));

    for (int step = 0; step < m_num_step; ++step) {
        controller.step();
    }

    EXPECT_CALL(*m_level_agent[root_level], report_header()).WillOnce(Return(m_agent_report));
    EXPECT_CALL(*m_level_agent[0], report_node()).WillOnce(Return(m_agent_report));
    EXPECT_CALL(*m_level_agent[0], report_region()).WillOnce(Return(m_region_names));
    EXPECT_CALL(*m_reporter, generate(_, _, _, _, _, _, _));
    EXPECT_CALL(*m_tracer, flush());
    controller.generate();

    std::set<int> send_down_levels {1, 0};
    std::set<int> recv_down_levels {1, 0};
    std::set<int> send_up_levels {0, 1};
    std::set<int> recv_up_levels {0, 1};
    EXPECT_THAT(send_down_levels, ContainerEq(m_tree_comm->levels_sent_down()));
    EXPECT_THAT(recv_down_levels, ContainerEq(m_tree_comm->levels_rcvd_down()));
    EXPECT_THAT(send_up_levels, ContainerEq(m_tree_comm->levels_sent_up()));
    EXPECT_THAT(recv_up_levels, ContainerEq(m_tree_comm->levels_rcvd_up()));
}

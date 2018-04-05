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
#include <iostream>
#include <sstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Kontroller.hpp"

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
using geopm::IComm;
using geopm::IAgent;
using testing::NiceMock;
using testing::_;
using testing::Return;
using testing::AtLeast;


class KontrollerTestMockPlatformIO : public MockPlatformIO
{
    public:
        KontrollerTestMockPlatformIO()
        {
            ON_CALL(*this, agg_function(_))
                .WillByDefault(Return(IPlatformIO::agg_sum));

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



class KontrollerTest : public ::testing::Test
{
    protected:
        void SetUp();

        std::string m_agent_name = "temp";
        int m_num_send_up = 4;//3;
        int m_num_send_down = 2;
        int m_num_level_ctl = 2;
        int m_root_level = 1;
        std::shared_ptr<MockComm> m_comm;
        MockPlatformTopo m_topo;
        KontrollerTestMockPlatformIO m_platform_io;
        std::shared_ptr<MockApplicationIO> m_application_io;
        MockTreeComm *m_tree_comm;
        MockReporter *m_reporter;
        MockTracer *m_tracer;
        std::vector<MockAgent*> m_level_agent;
        std::vector<std::unique_ptr<IAgent> > m_agents;
        MockManagerIOSampler *m_manager_io;

        int m_num_step = 3;
};

void KontrollerTest::SetUp()
{
    // static policy agent signals
    m_platform_io.add_supported_signal({"TIME", IPlatformTopo::M_DOMAIN_BOARD, 0}, 99);
    m_platform_io.add_supported_signal({"POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0}, 4545);
    m_platform_io.add_supported_signal({"FREQUENCY", IPlatformTopo::M_DOMAIN_BOARD, 0}, 333);
    m_platform_io.add_supported_signal({"REGION_PROGRESS", IPlatformTopo::M_DOMAIN_BOARD, 0}, 0.5);

    // toggle to suppress warnings; this is not the correct set of expect call for this test
#if 1
    EXPECT_CALL(m_platform_io, push_signal(_, _, _)).Times(AtLeast(0));
    EXPECT_CALL(m_platform_io, sample(_)).Times(AtLeast(0));
    EXPECT_CALL(m_platform_io, read_signal(_, _, _)).Times(AtLeast(0));
    EXPECT_CALL(m_platform_io, agg_function(_)).Times(AtLeast(0));
    EXPECT_CALL(m_platform_io, push_control(_, _, _)).Times(AtLeast(0));
    EXPECT_CALL(m_platform_io, adjust(_, _)).Times(AtLeast(0));
#endif

    m_comm = std::make_shared<MockComm>();
    m_application_io = std::make_shared<MockApplicationIO>();
    m_tree_comm = new MockTreeComm();
    m_manager_io = new MockManagerIOSampler();
    m_reporter = new MockReporter();
    m_tracer = new MockTracer();

    for (int level = 0; level < m_num_level_ctl; ++level) {
        auto tmp = new MockAgent();
        EXPECT_CALL(*tmp, init(level));
        tmp->init(level);
        m_level_agent.push_back(tmp);

        EXPECT_CALL(*m_tree_comm, level_size(level)).WillOnce(Return(1));
        m_agents.emplace_back(m_level_agent[level]);
    }
}

// TODO: single node kontroller test

TEST_F(KontrollerTest, main)
{
    Kontroller kontroller(m_comm, m_topo, m_platform_io,
                          m_agent_name, m_num_send_down, m_num_send_up,
                          std::unique_ptr<MockTreeComm>(m_tree_comm),
                          m_application_io,
                          std::unique_ptr<MockReporter>(m_reporter),
                          std::unique_ptr<MockTracer>(m_tracer),
                          std::move(m_agents),
                          std::unique_ptr<MockManagerIOSampler>(m_manager_io));

    std::vector<IPlatformIO::m_request_s> m_trace_cols {
        {"COL1", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"COL2", IPlatformTopo::M_DOMAIN_BOARD, 0}};
    EXPECT_CALL(*m_level_agent[0], trace_columns()).WillOnce(Return(m_trace_cols));
    EXPECT_CALL(*m_tracer, columns(_));
    for (auto col : m_trace_cols) {
        EXPECT_CALL(m_platform_io, push_signal(col.name, col.domain_type, col.domain_idx));
    }
    kontroller.setup_trace();

    EXPECT_CALL(m_platform_io, read_batch()).Times(m_num_step);
    EXPECT_CALL(m_platform_io, write_batch()).Times(m_num_step);
    EXPECT_CALL(*m_application_io, update(_)).Times(m_num_step);
    std::vector<double> manager_sample = {8.8, 9.9};
    ASSERT_EQ(m_num_send_down, (int)manager_sample.size());
    EXPECT_CALL(*m_manager_io, sample()).Times(m_num_step)
        .WillRepeatedly(Return(manager_sample));
    EXPECT_CALL(*m_level_agent[0], adjust_platform(_)).Times(m_num_step);
    EXPECT_CALL(*m_level_agent[0], sample_platform(_)).Times(m_num_step);
    EXPECT_CALL(*m_level_agent[0], wait()).Times(m_num_step);
    /// @todo set expectations for which agents should call ascend/descend

    for (int step = 0; step < m_num_step; ++step) {
        kontroller.step();
    }

    EXPECT_CALL(*m_level_agent[m_root_level], report_header());
    for (auto agent : m_level_agent) {
        EXPECT_CALL(*agent, report_node());
    }
    std::map<uint64_t, std::string> region_names {};
    EXPECT_CALL(*m_level_agent[0], report_region()).WillOnce(Return(region_names));
    EXPECT_CALL(*m_reporter, generate(_, _, _, _, _, _));
    EXPECT_CALL(*m_tracer, flush());
    kontroller.generate();

}

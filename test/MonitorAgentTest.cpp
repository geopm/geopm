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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MonitorAgent.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "Helper.hpp"

using geopm::IPlatformTopo;
using geopm::IPlatformIO;
using geopm::MonitorAgent;
using ::testing::_;
using ::testing::Return;

class MonitorAgentTest : public ::testing::Test
{
    protected:
        enum signal_idx_e {
            M_OTHER,  // signal not used by this agent; index may not start at 0
            M_TIME,
            M_POWER_PACKAGE,
            M_FREQUENCY,
            M_REGION_PROGRESS
        };
        MonitorAgentTest();
        void SetUp();
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        IPlatformIO &m_plat_io_ref;
        IPlatformTopo &m_plat_topo_ref;
        std::unique_ptr<geopm::MonitorAgent> m_agent;
};

MonitorAgentTest::MonitorAgentTest()
    : m_plat_io_ref(m_platform_io)
    , m_plat_topo_ref(m_platform_topo)
{

}

void MonitorAgentTest::SetUp()
{
    // all signals are over entire board for now
    ON_CALL(m_platform_io, push_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_TIME));
    ON_CALL(m_platform_io, push_signal("POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_POWER_PACKAGE));
    ON_CALL(m_platform_io, push_signal("FREQUENCY", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_FREQUENCY));
    ON_CALL(m_platform_io, push_signal("REGION_PROGRESS", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_REGION_PROGRESS));

    EXPECT_CALL(m_platform_io, push_signal("TIME", _, _));
    EXPECT_CALL(m_platform_io, push_signal("POWER_PACKAGE", _, _));
    EXPECT_CALL(m_platform_io, push_signal("FREQUENCY", _, _));
    EXPECT_CALL(m_platform_io, push_signal("REGION_PROGRESS", _, _));

    // does not necessarily match PlatformIO, but Agent should call
    // these and use whatever function is returned
    EXPECT_CALL(m_platform_io, agg_function("TIME"))
        .WillOnce(Return(IPlatformIO::agg_max));
    EXPECT_CALL(m_platform_io, agg_function("POWER_PACKAGE"))
        .WillOnce(Return(IPlatformIO::agg_sum));
    EXPECT_CALL(m_platform_io, agg_function("FREQUENCY"))
        .WillOnce(Return(IPlatformIO::agg_average));
    EXPECT_CALL(m_platform_io, agg_function("REGION_PROGRESS"))
        .WillOnce(Return(IPlatformIO::agg_min));

    m_agent = geopm::make_unique<MonitorAgent>(m_plat_io_ref, m_plat_topo_ref);
}

TEST_F(MonitorAgentTest, fixed_signal_list)
{
    // default list we collect with this agent
    // if this list changes, update the mocked platform for this test
    std::vector<std::string> expected_signals = {"TIME", "POWER_PACKAGE", "FREQUENCY", "REGION_PROGRESS"};
    EXPECT_EQ(expected_signals, m_agent->sample_names());
}

TEST_F(MonitorAgentTest, all_signals_in_trace)
{
    auto signals = m_agent->sample_names();
    auto trace_col = m_agent->trace_columns();
    ASSERT_EQ(signals.size(), trace_col.size());
    for (size_t idx = 0; idx < signals.size(); ++idx) {
        EXPECT_EQ(signals[idx], trace_col[idx].name);
        // for now everything is board domain
        EXPECT_EQ(IPlatformTopo::M_DOMAIN_BOARD, trace_col[idx].domain_type);
        EXPECT_EQ(0, trace_col[idx].domain_idx);
    }
}

TEST_F(MonitorAgentTest, sample_platform)
{
    std::vector<double> expected_value {456, 789, 1234, 5678};
    EXPECT_CALL(m_platform_io, sample(M_TIME))
        .WillOnce(Return(expected_value[0]));
    EXPECT_CALL(m_platform_io, sample(M_POWER_PACKAGE))
        .WillOnce(Return(expected_value[1]));
    EXPECT_CALL(m_platform_io, sample(M_FREQUENCY))
        .WillOnce(Return(expected_value[2]));
    EXPECT_CALL(m_platform_io, sample(M_REGION_PROGRESS))
        .WillOnce(Return(expected_value[3]));

    std::vector<double> result(expected_value.size());
    m_agent->sample_platform(result);
    EXPECT_EQ(expected_value, result);
}

TEST_F(MonitorAgentTest, descend_nothing)
{
    std::vector<std::string> expected_policy_names = {};
    EXPECT_EQ(expected_policy_names, m_agent->policy_names());
}

TEST_F(MonitorAgentTest, ascend_aggregates_signals)
{
    std::vector<std::vector<double> > input = {
        {5, 3, 8, 1},
        {6, 4, 9, 0.8},
        {7, 5, 10, 0.5}
    };
    std::vector<double> expected = {
        7,   // max
        12,  // sum
        9,   // average
        0.5, // min
    };
    std::vector<double> result(expected.size());;
    m_agent->ascend(input, result);

    EXPECT_EQ(expected, result);
}

TEST_F(MonitorAgentTest, custom_signals)
{
    ON_CALL(m_platform_io, agg_function(_)).WillByDefault(Return(IPlatformIO::agg_sum));


    auto err = setenv("GEOPM_MONITOR_AGENT_SIGNALS", "test1,test2,,test3", 0);
    ASSERT_EQ(0, err);

    std::vector<IPlatformIO::m_request_s> expected = {
        {"test1", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"test2", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"test3", IPlatformTopo::M_DOMAIN_BOARD, 0},
    };

    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_CALL(m_platform_io, push_signal(expected[i].name,
                                               expected[i].domain_type,
                                               expected[i].domain_idx));
        EXPECT_CALL(m_platform_io, agg_function(expected[i].name));
    }

    MonitorAgent agent(m_plat_io_ref, m_plat_topo_ref);
    auto cols = agent.trace_columns();
    ASSERT_EQ(expected.size(), cols.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i].name, cols[i].name);
        EXPECT_EQ(expected[i].domain_type, cols[i].domain_type);
        EXPECT_EQ(expected[i].domain_idx, cols[i].domain_idx);
    }

    unsetenv("GEOPM_MONITOR_AGENT_SIGNALS");
}

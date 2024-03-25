/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MonitorAgent.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockWaiter.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"

using geopm::PlatformTopo;
using geopm::MonitorAgent;
using ::testing::_;
using ::testing::Return;

class MonitorAgentTest : public ::testing::Test
{
    protected:
        void SetUp();
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        std::unique_ptr<geopm::MonitorAgent> m_agent;
        std::shared_ptr<MockWaiter> m_waiter;
};

void MonitorAgentTest::SetUp()
{
    m_waiter = std::make_unique<MockWaiter>();
    m_agent = std::make_unique<MonitorAgent>(m_platform_io, m_platform_topo, m_waiter);
}

TEST_F(MonitorAgentTest, sample_names)
{
    std::vector<std::string> expected_sample_names = {};
    EXPECT_EQ(expected_sample_names, m_agent->sample_names());
}

TEST_F(MonitorAgentTest, policy_names)
{
    std::vector<std::string> expected_policy_names = {};
    EXPECT_EQ(expected_policy_names, m_agent->policy_names());
}

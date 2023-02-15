/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MonitorAgent.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
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
};

void MonitorAgentTest::SetUp()
{
    m_agent = geopm::make_unique<MonitorAgent>(m_platform_io, m_platform_topo);
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

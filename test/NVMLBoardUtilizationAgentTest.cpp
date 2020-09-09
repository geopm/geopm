/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "NVMLBoardUtilizationAgent.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "Helper.hpp"
#include "Agg.hpp"

using geopm::PlatformTopo;
using geopm::NVMLBoardUtilizationAgent;
using ::testing::_;
using ::testing::Return;

class NVMLBoardUtilizationAgentTest : public ::testing::Test
{
    protected:
        void SetUp();
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        std::unique_ptr<geopm::NVMLBoardUtilizationAgent> m_agent;
};

void NVMLBoardUtilizationAgentTest::SetUp()
{
    m_agent = geopm::make_unique<NVMLBoardUtilizationAgent>(m_platform_io, m_platform_topo);
}

TEST_F(NVMLBoardUtilizationAgentTest, sample_names)
{
    std::vector<std::string> expected_sample_names = {};
    EXPECT_EQ(expected_sample_names, m_agent->sample_names());
}

TEST_F(NVMLBoardUtilizationAgentTest, policy_names)
{

    std::vector<std::string> expected_policy_names = {"ACCELERATOR_UTIL_THRESH_0",
                                                      "ACCELERATOR_FREQUENCY_SUB_THRESH_0",
                                                      "XEON_FREQUENCY_SUB_THRESH_0",
                                                      "ACCELERATOR_UTIL_THRESH_1",
                                                      "ACCELERATOR_FREQUENCY_SUB_THRESH_1",
                                                      "XEON_FREQUENCY_SUB_THRESH_1",
                                                      "ACCELERATOR_FREQUENCY_ABOVE_THRESH_1",
                                                      "XEON_FREQUENCY_ABOVE_THRESH_1"};
    EXPECT_EQ(expected_policy_names, m_agent->policy_names());
}

TEST_F(NVMLBoardUtilizationAgentTest, plugin_name)
{
    EXPECT_EQ("nvml_board_utilization", NVMLBoardUtilizationAgent::plugin_name());
}


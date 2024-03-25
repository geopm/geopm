/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "EndpointPolicyTracerImp.hpp"
#include "MockPlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"

using testing::Return;

class EndpointPolicyTracerTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        MockPlatformIO m_platform_io;
        std::string m_path = "test.policytrace";
        std::vector<std::string> m_agent_policy;
        int m_time_signal = 42;
};

void EndpointPolicyTracerTest::SetUp(void)
{
    m_agent_policy = {"power", "freq", "mode"};
}

TEST_F(EndpointPolicyTracerTest, construct_update_destruct)
{
    // Test that the tracer samples time
    EXPECT_CALL(m_platform_io, push_signal("TIME", GEOPM_DOMAIN_BOARD, 0))
            .WillOnce(Return(m_time_signal));
    EXPECT_CALL(m_platform_io, sample(m_time_signal));
    // Test that the constructor and update methods do not throw
    std::unique_ptr<geopm::EndpointPolicyTracer> tracer =
        geopm::make_unique<geopm::EndpointPolicyTracerImp>(2, true, m_path, m_platform_io, m_agent_policy);
    std::vector<double> policy {77.7, 80.6, 44.5};
    tracer->update(policy);
    // Test that a file was created by deleting it without error
    int err = unlink(m_path.c_str());
    EXPECT_EQ(0, err);
}

TEST_F(EndpointPolicyTracerTest, format)
{
    EXPECT_CALL(m_platform_io, push_signal("TIME", GEOPM_DOMAIN_BOARD, 0))
            .WillOnce(Return(m_time_signal));
    geopm::EndpointPolicyTracerImp tracer(2, true, m_path, m_platform_io, m_agent_policy);

    for (int ii = 0; ii < 5; ++ii) {
        EXPECT_CALL(m_platform_io, sample(m_time_signal))
            .WillOnce(Return(ii));
        tracer.update({100.0 + ii, 1e9 * ii, 5.5 * ii});
    }
    std::string output = geopm::read_file(m_path);
    std::vector<std::string> output_lines = geopm::string_split(output, "\n");
    std::vector<std::string> expect_lines = {
        "timestamp|power|freq|mode",
        "0|100|0|0",
        "1|101|1000000000|5.5",
        "2|102|2000000000|11",
        "3|103|3000000000|16.5",
        "4|104|4000000000|22"
    };
    auto expect_it = expect_lines.begin();
    for (const auto &output_it : output_lines) {
        if (output_it[0] != '#' && output_it.size()) {
            EXPECT_EQ(*expect_it, output_it);
            ++expect_it;
        }
    }
    int err = unlink(m_path.c_str());
    EXPECT_EQ(0, err);
}

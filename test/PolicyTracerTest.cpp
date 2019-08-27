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

#include <memory>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "PolicyTracer.hpp"
#include "MockPlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"

using testing::Return;

class PolicyTracerTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        MockPlatformIO m_platform_io;
        std::string m_path = "test.policytrace";
        std::vector<std::string> m_agent_policy;
        int m_time_signal = 42;
};

void PolicyTracerTest::SetUp(void)
{
    m_agent_policy = {"power", "freq", "mode"};
}

TEST_F(PolicyTracerTest, construct_update_destruct)
{
    // Test that the tracer samples time
    EXPECT_CALL(m_platform_io, push_signal("TIME", GEOPM_DOMAIN_BOARD, 0))
            .WillOnce(Return(m_time_signal));
    EXPECT_CALL(m_platform_io, sample(m_time_signal));
    // Test that the constructor and update methods do not throw
    std::unique_ptr<geopm::PolicyTracer> tracer = geopm::make_unique<geopm::PolicyTracerImp>(2, true, m_path, m_platform_io, m_agent_policy);
    std::vector<double> policy {77.7, 80.6, 44.5};
    tracer->update(policy);
    // Test that a file was created by deleting it without error
    int err = unlink(m_path.c_str());
    EXPECT_EQ(0, err);
}

TEST_F(PolicyTracerTest, format)
{
    EXPECT_CALL(m_platform_io, push_signal("TIME", GEOPM_DOMAIN_BOARD, 0))
            .WillOnce(Return(m_time_signal));
    geopm::PolicyTracerImp tracer(2, true, m_path, m_platform_io, m_agent_policy);

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

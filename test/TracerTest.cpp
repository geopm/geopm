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

#include <fstream>
#include <sstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Helper.hpp"
#include "Tracer.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "geopm_internal.h"
#include "geopm_hash.h"
#include "geopm_test.hpp"

using geopm::Tracer;
using geopm::IPlatformIO;
using geopm::IPlatformTopo;
using testing::_;
using testing::Return;
using testing::HasSubstr;

class TracerTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        void TearDown(void);
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        std::string m_path = "test.trace";
        std::string m_hostname = "myhost";
        std::string m_agent = "myagent";
        std::string m_profile = "myprofile";
        std::string m_start_time = "Tue Nov  6 08:00:00 2018";
        std::vector<IPlatformIO::m_request_s> m_default_cols;
        std::vector<std::string> m_extra_cols;
        std::string m_extra_cols_str;
        const int m_num_extra_cols = 3;
};

void TracerTest::SetUp(void)
{
    std::remove(m_path.c_str());

    EXPECT_CALL(m_platform_topo, num_domain(IPlatformTopo::M_DOMAIN_CPU))
        .WillOnce(Return(2));

    m_default_cols = {
        {"TIME", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"EPOCH_COUNT", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"REGION_HASH", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"REGION_HINT", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"REGION_PROGRESS", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"REGION_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"ENERGY_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"ENERGY_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"FREQUENCY", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"CYCLES_THREAD", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"CYCLES_REFERENCE", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"TEMPERATURE_CORE", IPlatformTopo::M_DOMAIN_BOARD, 0}
    };
    m_extra_cols_str = "EXTRA,EXTRA_SPECIAL@cpu";
    m_extra_cols = geopm::string_split(m_extra_cols_str, ",");

    int idx = 0;
    for (auto cc : m_default_cols) {
        EXPECT_CALL(m_platform_io, push_signal(cc.name, cc.domain_type, cc.domain_idx))
            .WillOnce(Return(idx));
        ++idx;
    }
    EXPECT_CALL(m_platform_io, push_signal("EXTRA", IPlatformTopo::M_DOMAIN_BOARD, 0))
            .WillOnce(Return(idx));
    ++idx;
    EXPECT_CALL(m_platform_io, push_signal("EXTRA_SPECIAL", IPlatformTopo::M_DOMAIN_CPU, 0))
        .WillOnce(Return(idx));
    ++idx;
    EXPECT_CALL(m_platform_io, push_signal("EXTRA_SPECIAL", IPlatformTopo::M_DOMAIN_CPU, 1))
        .WillOnce(Return(idx));
}

void TracerTest::TearDown(void)
{
    std::remove((m_path + "-" + m_hostname).c_str());
}

void check_trace(std::istream &expected, std::istream &result);

TEST_F(TracerTest, columns)
{
    Tracer tracer(m_start_time, m_path, m_hostname, m_agent, m_profile, true, m_platform_io, m_platform_topo, m_extra_cols_str, 1);

    // columns from agent will be printed as-is
    std::vector<std::string> agent_cols {"col1", "col2"};

    tracer.columns(agent_cols);
    tracer.flush();

    std::string expected_header = "# \"geopm_version\"\n"
                                  "# \"start_time\" : \"" + m_start_time + "\"\n" +
                                  "# \"profile_name\" : \"" + m_profile + "\"\n" +
                                  "# \"node_name\" : \"" + m_hostname + "\"\n" +
                                  "# \"agent\" : \"" + m_agent + "\"\n";
    std::string expected_str = expected_header +
        "time|epoch_count|region_hash|region_hint|region_progress|region_runtime|energy_package|energy_dram|"
        "power_package|power_dram|frequency|cycles_thread|cycles_reference|temperature_core|"
        "extra|extra_special-cpu-0|extra_special-cpu-1|"
        "col1|col2\n";
    std::istringstream expected(expected_str);
    std::ifstream result(m_path + "-" + m_hostname);
    ASSERT_TRUE(result.good()) << strerror(errno);
    check_trace(expected, result);
}

TEST_F(TracerTest, update_samples)
{
    Tracer tracer(m_start_time, m_path, m_hostname, m_agent, m_profile, true, m_platform_io, m_platform_topo, m_extra_cols_str, 1);
    int idx = 0;
    for (auto cc : m_default_cols) {
        EXPECT_CALL(m_platform_io, sample(idx))
            .WillOnce(Return(idx + 0.5));
        ++idx;
    }

    for (int count = 0; count < m_num_extra_cols; ++count) {
        EXPECT_CALL(m_platform_io, sample(idx))
            .WillOnce(Return(idx + 0.7));
        ++idx;
    }

    std::vector<std::string> agent_cols {"col1", "col2"};
    std::vector<double> agent_vals {88.8, 77.7};

    tracer.columns(agent_cols);
    tracer.update(agent_vals, {});
    tracer.flush();
    tracer.update(agent_vals, {}); // no additional samples after flush

    std::string expected_str = "\n\n\n\n\n\n"
        "5.0e-01|1.5e+00|0x0000000000000002|0x0000000000000003|4.5|5.5e+00|6.5e+00|7.5e+00|8.5e+00|9.5e+00|1.0e+01|1.2e+01|1.2e+01|1.4e+01|1.5e+01|1.6e+01|1.7e+01|8.9e+01|7.8e+0\n";
    std::istringstream expected(expected_str);
    std::ifstream result(m_path + "-" + m_hostname);
    ASSERT_TRUE(result.good()) << strerror(errno);
    check_trace(expected, result);
}

TEST_F(TracerTest, region_entry_exit)
{
    Tracer tracer(m_start_time, m_path, m_hostname, m_agent, m_profile, true, m_platform_io, m_platform_topo, m_extra_cols_str, 1);
    EXPECT_CALL(m_platform_io, sample(_)).Times(m_default_cols.size() + m_num_extra_cols)
        .WillOnce(Return(2.2))                        // time
        .WillOnce(Return(0.0))                        // epoch_count
        .WillOnce(Return(0x123))                      // region hash
        .WillOnce(Return(GEOPM_REGION_HINT_UNKNOWN))  // region hint
        .WillOnce(Return(0.0))  // progress; should cause one region entry to be skipped
        .WillRepeatedly(Return(2.2));

    std::vector<std::string> agent_cols {"col1", "col2"};
    std::vector<double> agent_vals {88.8, 77.7};

    std::list<geopm_region_info_s> short_regions = {
        {0x123, GEOPM_REGION_HINT_UNKNOWN, 0.0, 3.2},
        {0x123, GEOPM_REGION_HINT_UNKNOWN, 1.0, 3.2},
        {0x345, GEOPM_REGION_HINT_UNKNOWN, 0.0, 3.2},
        {0x456, GEOPM_REGION_HINT_UNKNOWN, 1.0, 3.2},
        {0x345, GEOPM_REGION_HINT_UNKNOWN, 1.0, 3.2},
    };
    tracer.columns(agent_cols);
    tracer.update(agent_vals, short_regions);
    tracer.flush();
    tracer.update(agent_vals, short_regions); // no additional samples after flush
    std::string expected_str ="\n\n\n\n\n"
        "\n" // header
        "2.2e+00|0.0e+00|0x0000000000000123|0x0000000100000000|0.0|3.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|8.9e+01|7.8e+01\n"
        "2.2e+00|0.0e+00|0x0000000000000123|0x0000000100000000|1.0|3.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|8.9e+01|7.8e+01\n"
        "2.2e+00|0.0e+00|0x0000000000000345|0x0000000100000000|0.0|3.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|8.9e+01|7.8e+01\n"
        "2.2e+00|0.0e+00|0x0000000000000456|0x0000000100000000|1.0|3.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|8.9e+01|7.8e+01\n"
        "2.2e+00|0.0e+00|0x0000000000000345|0x0000000100000000|1.0|3.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|2.2e+00|8.9e+01|7.8e+01\n"
        "\n"; // sample

     std::istringstream expected(expected_str);
     std::ifstream result(m_path + "-" + m_hostname);
     ASSERT_TRUE(result.good()) << strerror(errno);
     check_trace(expected, result);
}

/// @todo This is shared with ReporterTest; can be put in common file
void check_trace(std::istream &expected, std::istream &result)
{
    char exp_line[1024];
    char res_line[1024];
    expected.getline(exp_line, 1024);
    result.getline(res_line, 1024);
    int line = 1;
    while (expected.good() && result.good()) {
        ASSERT_THAT(std::string(res_line), HasSubstr(exp_line)) << " on line " << line;
        expected.getline(exp_line, 1024);
        result.getline(res_line, 1024);
        ++line;
    }
    if (expected.good() != result.good()) {
        std::ostringstream message;
        message << "Different length strings." << std::endl;
        message << "Remaining expected:" << std::endl;
        message << "--------" << std::endl;
        while (expected.good()) {
            expected.getline(exp_line, 1024);
            message << exp_line << std::endl;
        }
        message << "--------" << std::endl;
        message << "Remaining result:" << std::endl;
        message << "--------" << std::endl;
        while (result.good()) {
            result.getline(res_line, 1024);
            message << res_line << std::endl;
        }
        message << "--------" << std::endl;
        FAIL() << message.str();
    }
}

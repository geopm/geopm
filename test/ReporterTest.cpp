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

#include <sstream>
#include <fstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Reporter.hpp"
#include "MockPlatformIO.hpp"
#include "MockApplicationIO.hpp"
#include "MockTreeComm.hpp"
#include "Helper.hpp"
#include "geopm_hash.h"
#include "config.h"

using geopm::Reporter;
using testing::HasSubstr;
using testing::Return;
using testing::_;

class ReporterTest : public testing::Test
{
    protected:
        enum {
            M_ENERGY_IDX,
            M_REGION_ENERGY_IDX,
            M_CLK_CORE_IDX,
            M_CLK_REF_IDX,
            M_REGION_CLK_CORE_IDX,
            M_REGION_CLK_REF_IDX,
        };
        ReporterTest();
        void TearDown(void);
        std::string m_report_name = "test_reporter.out";

        MockPlatformIO m_platform_io;
        MockApplicationIO m_application_io;
        MockTreeComm m_tree_comm;
        std::unique_ptr<Reporter> m_reporter;
        std::string m_profile_name = "my profile";
        std::set<std::string> m_region_set = {"all2all", "model-init"};
        std::map<uint64_t, double> m_region_runtime = {
            {geopm_crc32_str(0, "all2all"), 33.33},
            {geopm_crc32_str(0, "model-init"), 22.11}
        };
        std::map<uint64_t, double> m_region_mpi_time = {
            {geopm_crc32_str(0, "all2all"), 3.4},
            {geopm_crc32_str(0, "model-init"), 5.6}
        };
        std::map<uint64_t, double> m_region_count = {
            {geopm_crc32_str(0, "all2all"), 20},
            {geopm_crc32_str(0, "model-init"), 1}
        };

        std::map<uint64_t, double> m_region_energy = {
            {geopm_crc32_str(0, "all2all"), 777},
            {geopm_crc32_str(0, "model-init"), 888}
        };
        std::map<uint64_t, double> m_region_clk_core = {
            {geopm_crc32_str(0, "all2all"), 4545},
            {geopm_crc32_str(0, "model-init"), 5656}
        };
        std::map<uint64_t, double> m_region_clk_ref = {
            {geopm_crc32_str(0, "all2all"), 5555},
            {geopm_crc32_str(0, "model-init"), 6666}
        };
};

ReporterTest::ReporterTest()
{
    ON_CALL(m_application_io, profile_name())
        .WillByDefault(Return(m_profile_name));
    ON_CALL(m_application_io, region_name_set())
        .WillByDefault(Return(m_region_set));

    EXPECT_CALL(m_platform_io, push_signal("ENERGY_PACKAGE", _, _))
        .WillOnce(Return(M_ENERGY_IDX));
    EXPECT_CALL(m_platform_io, push_region_signal(M_ENERGY_IDX, _, _))
        .WillOnce(Return(M_REGION_ENERGY_IDX));
    EXPECT_CALL(m_platform_io, push_signal("CYCLES_REFERENCE", _, _))
        .WillOnce(Return(M_CLK_REF_IDX));
    EXPECT_CALL(m_platform_io, push_region_signal(M_CLK_REF_IDX, _, _))
        .WillOnce(Return(M_REGION_CLK_REF_IDX));
    EXPECT_CALL(m_platform_io, push_signal("CYCLES_THREAD", _, _))
        .WillOnce(Return(M_CLK_CORE_IDX));
    EXPECT_CALL(m_platform_io, push_region_signal(M_CLK_CORE_IDX, _, _))
        .WillOnce(Return(M_REGION_CLK_CORE_IDX));

    m_reporter = geopm::make_unique<Reporter>(m_report_name, m_platform_io);
}

void ReporterTest::TearDown(void)
{
    //std::remove(m_report_name.c_str());
}

void check_report(std::istream &expected, std::istream &result);

TEST_F(ReporterTest, generate)
{
    EXPECT_CALL(m_application_io, profile_name());
    EXPECT_CALL(m_application_io, region_name_set());
    for (auto rid : m_region_runtime) {
        EXPECT_CALL(m_application_io, total_region_runtime(rid.first))
            .WillOnce(Return(rid.second));
    }
    for (auto rid : m_region_mpi_time) {
        EXPECT_CALL(m_application_io, total_region_mpi_runtime(rid.first))
            .WillOnce(Return(rid.second));
    }
    for (auto rid : m_region_count) {
        EXPECT_CALL(m_application_io, total_count(rid.first))
            .WillOnce(Return(rid.second));
    }

    for (auto rid : m_region_energy) {
        EXPECT_CALL(m_platform_io, region_sample(M_REGION_ENERGY_IDX, rid.first))
            .WillOnce(Return(rid.second));
    }
    for (auto rid : m_region_clk_core) {
        EXPECT_CALL(m_platform_io, region_sample(M_REGION_CLK_CORE_IDX, rid.first))
            .WillOnce(Return(rid.second));
    }
    for (auto rid : m_region_clk_ref) {
        EXPECT_CALL(m_platform_io, region_sample(M_REGION_CLK_REF_IDX, rid.first))
            .WillOnce(Return(rid.second));
    }


    EXPECT_CALL(m_application_io, total_app_runtime())
        .WillOnce(Return(44));
    EXPECT_CALL(m_application_io, total_app_mpi_runtime())
        .WillOnce(Return(22));
    EXPECT_CALL(m_application_io, total_app_energy())
        .WillOnce(Return(999));
    EXPECT_CALL(m_tree_comm, overhead_send())
        .WillOnce(Return(678 * 44));

    // Check for labels at start of line but ignore numbers
    // Note that region lines start with tab
    std::string expected = "#####\n"
        "Profile: " + m_profile_name + "\n"
        "Agent: my_agent\n"
        "Policy Mode:\n"
        "Tree Decider:\n"
        "Leaf Decider:\n"
        "Power Budget:\n"
        "\n"
        "Host:\n"
        "Region all2all (\n"
        "	runtime (sec): 33.33\n"
        "	energy (joules): 777\n"
        "	frequency (%): 81.81\n"
        "	mpi-runtime (sec): 3.4\n"
        "	count: 20\n"
        "Region model-init (\n"
        "	runtime (sec): 22.11\n"
        "	energy (joules): 888\n"
        "	frequency (%): 84.84\n"
        "	mpi-runtime (sec): 5.6\n"
        "	count: 1\n"
        "Application Totals:\n"
        "	runtime (sec): 44\n"
        "	energy (joules): 999\n"
        "	mpi-runtime (sec): 22\n"
        "	ignore-time (sec):\n"
        "	throttle time (%):\n"
        "	geopmctl memory HWM:\n"
        "	geopmctl network BW (B/sec): 678\n"
        "\n";

    std::istringstream exp_stream(expected);

    m_reporter->generate("my_agent", "agent_header", "node_report", {},
                         m_application_io,
                         nullptr,  // TODO: mock comm; not used
                         m_tree_comm);
    std::ifstream report(m_report_name);
    check_report(exp_stream, report);
}

void check_report(std::istream &expected, std::istream &result)
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

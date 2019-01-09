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

#include "PlatformTopo.hpp"
#include "Reporter.hpp"
#include "MockPlatformIO.hpp"
#include "MockRegionAggregator.hpp"
#include "MockApplicationIO.hpp"
#include "MockComm.hpp"
#include "MockTreeComm.hpp"
#include "Helper.hpp"
#include "geopm.h"
#include "geopm_internal.h"
#include "geopm_hash.h"
#include "config.h"

using geopm::Reporter;
using testing::HasSubstr;
using testing::Return;
using testing::_;
using testing::SaveArg;
using testing::SetArgPointee;

// Mock for gathering reports; assumes one node only
class ReporterTestMockComm : public MockComm
{
    public:
        void gather(const void *send_buf, size_t send_size, void *recv_buf,
                    size_t recv_size, int root) const override
        {
            *((size_t*)(recv_buf)) = *((size_t*)(send_buf));
        }
        void gatherv(const void *send_buf, size_t send_size, void *recv_buf,
                     const std::vector<size_t> &recv_sizes,
                     const std::vector<off_t> &rank_offset, int root) const override
        {
            memcpy(recv_buf, send_buf, send_size);
        }
};

class ReporterTest : public testing::Test
{
    protected:
        enum {
            M_TIME_IDX,
            M_ENERGY_PKG_IDX,
            M_ENERGY_DRAM_IDX,
            M_CLK_CORE_IDX,
            M_CLK_REF_IDX,
        };
        ReporterTest();
        void TearDown(void);
        std::string m_report_name = "test_reporter.out";

        MockPlatformIO m_platform_io;
        MockRegionAggregator *m_agg;  // freed with Reporter
        MockApplicationIO m_application_io;
        std::shared_ptr<ReporterTestMockComm> m_comm;
        MockTreeComm m_tree_comm;
        std::unique_ptr<Reporter> m_reporter;
        std::string m_start_time = "Tue Nov  6 08:00:00 2018";
        std::string m_profile_name = "my profile";
        std::set<std::string> m_region_set = {"all2all", "model-init"};
        std::map<uint64_t, double> m_region_runtime = {
            {geopm_crc32_str("all2all"), 33.33},
            {geopm_crc32_str("model-init"), 22.11},
            {GEOPM_REGION_ID_UNMARKED, 12.13}
        };
        std::map<uint64_t, double> m_region_mpi_time = {
            {geopm_crc32_str("all2all"), 3.4},
            {geopm_crc32_str("model-init"), 5.6},
            {GEOPM_REGION_ID_UNMARKED, 1.2},
            {GEOPM_REGION_ID_EPOCH, 4.2}
        };
        std::map<uint64_t, double> m_region_count = {
            {geopm_crc32_str("all2all"), 20},
            {geopm_crc32_str("model-init"), 1},
            {GEOPM_REGION_ID_EPOCH, 0}
        };
        std::map<uint64_t, double> m_region_rt = {
            {geopm_crc32_str("all2all"), 555},
            {geopm_crc32_str("model-init"), 333},
            {GEOPM_REGION_ID_UNMARKED, 444},
            {GEOPM_REGION_ID_EPOCH, 666}
        };
        std::map<uint64_t, double> m_region_energy = {
            {geopm_crc32_str("all2all"), 777},
            {geopm_crc32_str("model-init"), 888},
            {GEOPM_REGION_ID_UNMARKED, 222},
            {GEOPM_REGION_ID_EPOCH, 334}
        };
        std::map<uint64_t, double> m_region_clk_core = {
            {geopm_crc32_str("all2all"), 4545},
            {geopm_crc32_str("model-init"), 5656},
            {GEOPM_REGION_ID_UNMARKED, 3434},
            {GEOPM_REGION_ID_EPOCH, 7878}
        };
        std::map<uint64_t, double> m_region_clk_ref = {
            {geopm_crc32_str("all2all"), 5555},
            {geopm_crc32_str("model-init"), 6666},
            {GEOPM_REGION_ID_UNMARKED, 4444},
            {GEOPM_REGION_ID_EPOCH, 8888}
        };
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > m_region_agent_detail = {
            {geopm_crc32_str("all2all"), {{"agent stat", "1"}, {"agent other stat", "2"}}},
            {geopm_crc32_str("model-init"), {{"agent stat", "2"}}},
            {GEOPM_REGION_ID_UNMARKED, {{"agent stat", "3"}}},
            {GEOPM_REGION_ID_EPOCH, {{"agent stat", "4"}}},
            // hints should be ignored by reporter
            {geopm_crc32_str("all2all") | GEOPM_REGION_ID_MPI, {{"agent new stat", "5"}}},
            {geopm_crc32_str("model-init") | GEOPM_REGION_HINT_IGNORE, {{"agent new stat", "6"}}},

        };
};

ReporterTest::ReporterTest()
{
    m_agg = new MockRegionAggregator();

    ON_CALL(m_application_io, profile_name())
        .WillByDefault(Return(m_profile_name));
    ON_CALL(m_application_io, region_name_set())
        .WillByDefault(Return(m_region_set));

    EXPECT_CALL(*m_agg, init());
    EXPECT_CALL(*m_agg, push_signal_total("TIME", _, _))
        .WillOnce(Return(M_TIME_IDX));
    EXPECT_CALL(*m_agg, push_signal_total("ENERGY_PACKAGE", _, _))
        .WillOnce(Return(M_ENERGY_PKG_IDX));
    EXPECT_CALL(*m_agg, push_signal_total("ENERGY_DRAM", _, _))
        .WillOnce(Return(M_ENERGY_DRAM_IDX));
    EXPECT_CALL(*m_agg, push_signal_total("CYCLES_REFERENCE", _, _))
        .WillOnce(Return(M_CLK_REF_IDX));
    EXPECT_CALL(*m_agg, push_signal_total("CYCLES_THREAD", _, _))
        .WillOnce(Return(M_CLK_CORE_IDX));
    m_comm = std::make_shared<ReporterTestMockComm>();
    m_reporter = geopm::make_unique<Reporter>(m_start_time, m_report_name, m_platform_io, 0,
                                              std::unique_ptr<MockRegionAggregator>(m_agg));
    m_reporter->init();
}

void ReporterTest::TearDown(void)
{
    std::remove(m_report_name.c_str());
}

void check_report(std::istream &expected, std::istream &result);

TEST_F(ReporterTest, generate)
{
    EXPECT_CALL(m_application_io, report_name()).WillOnce(Return(m_report_name));
    EXPECT_CALL(m_application_io, profile_name());
    EXPECT_CALL(m_application_io, region_name_set());
    EXPECT_CALL(m_application_io, total_app_runtime()).WillOnce(Return(56));
    EXPECT_CALL(m_application_io, total_app_energy_pkg()).WillOnce(Return(2222));
    EXPECT_CALL(m_application_io, total_app_energy_dram()).WillOnce(Return(2222));
    EXPECT_CALL(m_application_io, total_app_runtime_mpi()).WillOnce(Return(45));
    EXPECT_CALL(m_application_io, total_app_runtime_ignore()).WillOnce(Return(0.7));
    EXPECT_CALL(m_application_io, total_epoch_runtime_ignore()).WillRepeatedly(Return(0.7));
    EXPECT_CALL(m_application_io, total_epoch_runtime()).WillOnce(Return(70.0));
    EXPECT_CALL(m_platform_io, read_signal("CPUINFO::FREQ_STICKER", geopm::IPlatformTopo::M_DOMAIN_BOARD, 0))
        .Times(4)
        .WillRepeatedly(Return(1.0));
    EXPECT_CALL(m_tree_comm, overhead_send()).WillOnce(Return(678 * 56));
    for (auto rid : m_region_runtime) {
        EXPECT_CALL(m_application_io, total_region_runtime(rid.first))
            .WillOnce(Return(rid.second));
    }
    for (auto rid : m_region_mpi_time) {
        EXPECT_CALL(m_application_io, total_region_runtime_mpi(rid.first))
            .WillOnce(Return(rid.second));
    }
    for (auto rid : m_region_count) {
        EXPECT_CALL(m_application_io, total_count(rid.first))
            .WillOnce(Return(rid.second));
    }
    for (auto rid : m_region_rt) {
        EXPECT_CALL(*m_agg, sample_total(M_TIME_IDX, rid.first))
            .WillOnce(Return(rid.second));
        EXPECT_CALL(*m_agg, sample_total(M_TIME_IDX, geopm_region_id_set_mpi(rid.first)))
            .WillOnce(Return(0.5));
    }
    for (auto rid : m_region_energy) {
        EXPECT_CALL(*m_agg, sample_total(M_ENERGY_PKG_IDX, rid.first))
            .WillOnce(Return(rid.second/2.0));
        EXPECT_CALL(*m_agg, sample_total(M_ENERGY_PKG_IDX, geopm_region_id_set_mpi(rid.first)))
            .WillOnce(Return(0.5));
        EXPECT_CALL(*m_agg, sample_total(M_ENERGY_DRAM_IDX, rid.first))
            .WillOnce(Return(rid.second/2.0));
        EXPECT_CALL(*m_agg, sample_total(M_ENERGY_DRAM_IDX, geopm_region_id_set_mpi(rid.first)))
            .WillOnce(Return(0.5));
    }
    for (auto rid : m_region_clk_core) {
        EXPECT_CALL(*m_agg, sample_total(M_CLK_CORE_IDX, rid.first))
            .WillOnce(Return(rid.second));
        EXPECT_CALL(*m_agg, sample_total(M_CLK_CORE_IDX, geopm_region_id_set_mpi(rid.first)))
            .WillOnce(Return(rid.second));
    }
    for (auto rid : m_region_clk_ref) {
        EXPECT_CALL(*m_agg, sample_total(M_CLK_REF_IDX, rid.first))
            .WillOnce(Return(rid.second));
        EXPECT_CALL(*m_agg, sample_total(M_CLK_REF_IDX, geopm_region_id_set_mpi(rid.first)))
            .WillOnce(Return(rid.second));
    }
    EXPECT_CALL(*m_comm, rank()).WillOnce(Return(0));
    EXPECT_CALL(*m_comm, num_rank()).WillOnce(Return(1));

    std::vector<std::pair<std::string, std::string> >  agent_header {
        {"one", "1"},
        {"two", "2"} };
    std::vector<std::pair<std::string, std::string> >  agent_node_report {
        {"three", "3"},
        {"four", "4"} };

    // Check for labels at start of line but ignore numbers
    // Note that region lines start with tab
    std::string expected = "#####\n"
        "Start Time: " + m_start_time + "\n"
        "Profile: " + m_profile_name + "\n"
        "Agent: my_agent\n"
        "one: 1\n"
        "two: 2\n"
        "\n"
        "Host:\n"
        "three: 3\n"
        "four: 4\n"
        "Region all2all (\n"
        "    runtime (sec): 33.33\n"
        "    sync-runtime (sec): 555.5\n"
        "    package-energy (joules): 389\n"
        "    dram-energy (joules): 389\n"
        "    frequency (%): 81.81\n"
        "    frequency (Hz): 0.818182\n"
        "    mpi-runtime (sec): 3.4\n"
        "    count: 20\n"
        "    agent stat: 1\n"
        "    agent other stat: 2\n"
        "    agent new stat: 5\n"
        "Region model-init (\n"
        "    runtime (sec): 22.11\n"
        "    sync-runtime (sec): 333.5\n"
        "    package-energy (joules): 444.5\n"
        "    dram-energy (joules): 444.5\n"
        "    frequency (%): 84.84\n"
        "    frequency (Hz): 0.848485\n"
        "    mpi-runtime (sec): 5.6\n"
        "    count: 1\n"
        "    agent stat: 2\n"
        "    agent new stat: 6\n"
        "Region unmarked-region (\n"
        "    runtime (sec): 12.13\n"
        "    sync-runtime (sec): 444\n"
        "    package-energy (joules): 111.5\n"
        "    dram-energy (joules): 111.5\n"
        "    frequency (%): 77.2727\n"
        "    frequency (Hz): 0.772727\n"
        "    mpi-runtime (sec): 1.2\n"
        "    count: 0\n"
        "    agent stat: 3\n"
        "Region epoch (\n"
        "    runtime (sec): 70\n"
        "    sync-runtime (sec): 666\n"
        "    package-energy (joules): 167\n"
        "    dram-energy (joules): 167\n"
        "    frequency (%): 88.6364\n"
        "    frequency (Hz): 0.886364\n"
        "    mpi-runtime (sec): 4.2\n"
        "    count: 0\n"
        "    agent stat: 4\n"
        "    epoch-runtime-ignore (sec): 0.7\n"
        "Application Totals:\n"
        "    runtime (sec): 56\n"
        "    package-energy (joules): 2222\n"
        "    dram-energy (joules): 2222\n"
        "    mpi-runtime (sec): 45\n"
        "    ignore-time (sec): 0.7\n"
        "    geopmctl memory HWM:\n"
        "    geopmctl network BW (B/sec): 678\n\n";

    std::istringstream exp_stream(expected);

    m_reporter->generate("my_agent", agent_header, agent_node_report, m_region_agent_detail,
                         m_application_io,
                         m_comm, m_tree_comm);
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

/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <sstream>
#include <fstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm/PlatformTopo.hpp"
#include "Reporter.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockSampleAggregator.hpp"
#include "MockProcessRegionAggregator.hpp"
#include "MockApplicationIO.hpp"
#include "MockComm.hpp"
#include "MockTreeComm.hpp"
#include "geopm/Helper.hpp"
#include "geopm_prof.h"
#include "geopm_hash.h"
#include "geopm_version.h"


using geopm::Reporter;
using geopm::ReporterImp;
using geopm::PlatformTopo;
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
            M_TIME_NETWORK_IDX,
            M_TIME_IGNORE_IDX,
            M_TIME_COMPUTE_IDX,
            M_TIME_MEMORY_IDX,
            M_TIME_IO_IDX,
            M_TIME_SERIAL_IDX,
            M_TIME_PARALLEL_IDX,
            M_TIME_UNKNOWN_IDX,
            M_TIME_UNSET_IDX,
            M_TIME_SPIN_IDX,
            M_ENERGY_PKG_IDX,
            M_DRAM_ENERGY_IDX,
            M_CLK_CORE_IDX,
            M_CLK_REF_IDX,
            M_ENERGY_PKG_ENV_IDX_0,
            M_ENERGY_PKG_ENV_IDX_1,
            M_EPOCH_COUNT_IDX,
            M_ENERGY_GPU_IDX,
            M_POWER_GPU_IDX,
            M_FREQUENCY_GPU_IDX,
            M_FREQUENCY_CPU_UNCORE_IDX,
        };
        ReporterTest();
        void TearDown(void);
        void generate_setup(void);
        std::string m_report_name = "test_reporter.out";

        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        std::shared_ptr<MockSampleAggregator> m_sample_agg;
        std::shared_ptr<MockProcessRegionAggregator>  m_region_agg;
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
        };
        std::map<uint64_t, double> m_region_network_time = {
            {geopm_crc32_str("all2all"), 3.4},
            {geopm_crc32_str("model-init"), 5.6},
            {GEOPM_REGION_HASH_UNMARKED, 1.2},
            {GEOPM_REGION_HASH_EPOCH, 4.2},
            {GEOPM_REGION_HASH_APP, 45}
        };
        std::map<uint64_t, double> m_region_ignore_time = {
            {geopm_crc32_str("all2all"), 3.5},
            {geopm_crc32_str("model-init"), 5.7},
            {GEOPM_REGION_HASH_UNMARKED, 1.3},
            {GEOPM_REGION_HASH_EPOCH, 4.3},
            {GEOPM_REGION_HASH_APP, 46}
        };
        std::map<uint64_t, double> m_region_count = {
            {geopm_crc32_str("all2all"), 20},
            {geopm_crc32_str("model-init"), 1},
            {GEOPM_REGION_HASH_EPOCH, 66}
        };
        std::map<uint64_t, double> m_region_sync_rt = {
            {geopm_crc32_str("all2all"), 555},
            {geopm_crc32_str("model-init"), 333},
            {GEOPM_REGION_HASH_UNMARKED, 444},
            {GEOPM_REGION_HASH_EPOCH, 70},
            {GEOPM_REGION_HASH_APP, 56}
        };
        std::map<uint64_t, double> m_region_energy = {
            {geopm_crc32_str("all2all"), 777},
            {geopm_crc32_str("model-init"), 888},
            {GEOPM_REGION_HASH_UNMARKED, 222},
            {GEOPM_REGION_HASH_EPOCH, 334},
            {GEOPM_REGION_HASH_APP, 4444}
        };
        std::map<uint64_t, double> m_region_frequency_cpu_uncore = {
            {geopm_crc32_str("all2all"), 755},
            {geopm_crc32_str("model-init"), 198},
            {GEOPM_REGION_HASH_UNMARKED, 421},
            {GEOPM_REGION_HASH_EPOCH, 653},
            {GEOPM_REGION_HASH_APP, 121213}
        };
        std::map<uint64_t, double> m_region_frequency_gpu = {
            {geopm_crc32_str("all2all"), 567},
            {geopm_crc32_str("model-init"), 890},
            {GEOPM_REGION_HASH_UNMARKED, 123},
            {GEOPM_REGION_HASH_EPOCH, 456},
            {GEOPM_REGION_HASH_APP, 74489}
        };
        std::map<uint64_t, double> m_region_power_gpu = {
            {geopm_crc32_str("all2all"), 764},
            {geopm_crc32_str("model-init"), 653},
            {GEOPM_REGION_HASH_UNMARKED, 211},
            {GEOPM_REGION_HASH_EPOCH, 432},
            {GEOPM_REGION_HASH_APP, 8992}
        };
        std::map<uint64_t, double> m_region_clk_core = {
            {geopm_crc32_str("all2all"), 4545},
            {geopm_crc32_str("model-init"), 5656},
            {GEOPM_REGION_HASH_UNMARKED, 3434},
            {GEOPM_REGION_HASH_EPOCH, 7878},
            {GEOPM_REGION_HASH_APP, 22222}
        };
        std::map<uint64_t, double> m_region_clk_ref = {
            {geopm_crc32_str("all2all"), 5555},
            {geopm_crc32_str("model-init"), 6666},
            {GEOPM_REGION_HASH_UNMARKED, 4444},
            {GEOPM_REGION_HASH_EPOCH, 8888},
            {GEOPM_REGION_HASH_APP, 33344}
        };
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > m_region_agent_detail = {
            {geopm_crc32_str("all2all"), {{"agent stat", "1"}, {"agent other stat", "2"}}},
            {geopm_crc32_str("model-init"), {{"agent stat", "2"}}},
            {GEOPM_REGION_HASH_UNMARKED, {{"agent stat", "3"}}},
        };
};

ReporterTest::ReporterTest()
{
    m_sample_agg = std::make_shared<MockSampleAggregator>();
    m_region_agg = std::make_shared<MockProcessRegionAggregator>();

    ON_CALL(m_application_io, profile_name())
        .WillByDefault(Return(m_profile_name));
    ON_CALL(m_application_io, region_name_set())
        .WillByDefault(Return(m_region_set));

    EXPECT_CALL(*m_sample_agg, push_signal("TIME", _, _))
        .WillRepeatedly(Return(M_TIME_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("TIME_HINT_NETWORK", _, _))
        .WillRepeatedly(Return(M_TIME_NETWORK_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("TIME_HINT_IGNORE", _, _))
        .WillRepeatedly(Return(M_TIME_IGNORE_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("TIME_HINT_COMPUTE", _, _))
        .WillRepeatedly(Return(M_TIME_COMPUTE_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("TIME_HINT_MEMORY", _, _))
        .WillRepeatedly(Return(M_TIME_MEMORY_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("TIME_HINT_IO", _, _))
        .WillRepeatedly(Return(M_TIME_IO_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("TIME_HINT_SERIAL", _, _))
        .WillRepeatedly(Return(M_TIME_SERIAL_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("TIME_HINT_PARALLEL", _, _))
        .WillRepeatedly(Return(M_TIME_PARALLEL_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("TIME_HINT_UNKNOWN", _, _))
        .WillRepeatedly(Return(M_TIME_UNKNOWN_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("TIME_HINT_UNSET", _, _))
        .WillRepeatedly(Return(M_TIME_UNSET_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("TIME_HINT_SPIN", _, _))
        .WillRepeatedly(Return(M_TIME_SPIN_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("CPU_ENERGY", GEOPM_DOMAIN_BOARD, 0))
        .WillRepeatedly(Return(M_ENERGY_PKG_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("DRAM_ENERGY", _, _))
        .WillRepeatedly(Return(M_DRAM_ENERGY_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("CPU_CYCLES_REFERENCE", _, _))
        .WillRepeatedly(Return(M_CLK_REF_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("CPU_CYCLES_THREAD", _, _))
        .WillRepeatedly(Return(M_CLK_CORE_IDX));
    EXPECT_CALL(m_platform_io, push_signal("EPOCH_COUNT", GEOPM_DOMAIN_BOARD, 0))
        .WillRepeatedly(Return(M_EPOCH_COUNT_IDX));

    // environment signals
    EXPECT_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillRepeatedly(Return(2));
    EXPECT_CALL(*m_sample_agg, push_signal("CPU_ENERGY", GEOPM_DOMAIN_PACKAGE, 0))
        .WillOnce(Return(M_ENERGY_PKG_ENV_IDX_0));
    EXPECT_CALL(*m_sample_agg, push_signal("CPU_ENERGY", GEOPM_DOMAIN_PACKAGE, 1))
        .WillOnce(Return(M_ENERGY_PKG_ENV_IDX_1));

    EXPECT_CALL(m_platform_io, read_signal("CPUINFO::FREQ_STICKER", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(1.0));

    m_comm = std::make_shared<ReporterTestMockComm>();
}

void ReporterTest::TearDown(void)
{
    std::remove(m_report_name.c_str());
}

void check_report(std::istream &expected, std::istream &result);

// Common settings for generate* tests
void ReporterTest::generate_setup(void)
{
    // ApplicationIO calls: to be removed
    EXPECT_CALL(m_application_io, report_name()).WillOnce(Return(m_report_name));
    EXPECT_CALL(m_application_io, profile_name()).WillOnce(Return(m_profile_name));
    EXPECT_CALL(m_application_io, region_name_set()).WillOnce(Return(m_region_set));

    // ProcessRegionAgregator
    EXPECT_CALL(*m_region_agg, update);
    for (auto rid : m_region_runtime) {
        EXPECT_CALL(*m_region_agg, get_runtime_average(rid.first))
            .WillOnce(Return(rid.second));
    }
    for (auto rid : m_region_count) {
        if (GEOPM_REGION_HASH_EPOCH == rid.first) {
            EXPECT_CALL(m_platform_io, sample(M_EPOCH_COUNT_IDX))
                .WillOnce(Return(rid.second));
        }
        else {
            EXPECT_CALL(*m_region_agg, get_count_average(rid.first))
                .WillOnce(Return(rid.second));
        }
    }

    // SampleAggregator
    EXPECT_CALL(*m_sample_agg, update);
    for (auto rid : m_region_network_time) {
        EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_NETWORK_IDX, rid.first))
            .WillRepeatedly(Return(rid.second));
    }
    for (auto rid : m_region_ignore_time) {
        EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_IGNORE_IDX, rid.first))
            .WillRepeatedly(Return(rid.second));
    }
    for (auto rid : m_region_sync_rt) {
        EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_IDX, rid.first))
            .WillRepeatedly(Return(rid.second));
    }
    EXPECT_CALL(*m_sample_agg, sample_application(M_TIME_IDX))
        .WillRepeatedly(Return(56));
    EXPECT_CALL(*m_sample_agg, sample_epoch(M_TIME_IDX))
        .WillRepeatedly(Return(70));

    for (auto rid : m_region_energy) {
        EXPECT_CALL(*m_sample_agg, sample_region(M_ENERGY_PKG_IDX, rid.first))
            .WillRepeatedly(Return(rid.second/2.0));
        EXPECT_CALL(*m_sample_agg, sample_region(M_DRAM_ENERGY_IDX, rid.first))
            .WillRepeatedly(Return(rid.second/2.0));
        EXPECT_CALL(*m_sample_agg, sample_region(M_ENERGY_PKG_ENV_IDX_0, rid.first))
            .WillRepeatedly(Return(rid.second/4.0));
        EXPECT_CALL(*m_sample_agg, sample_region(M_ENERGY_PKG_ENV_IDX_1, rid.first))
            .WillRepeatedly(Return(rid.second/4.0));
    }

    for (auto rid : m_region_clk_core) {
        EXPECT_CALL(*m_sample_agg, sample_region(M_CLK_CORE_IDX, rid.first))
            .WillRepeatedly(Return(rid.second));
    }
    for (auto rid : m_region_clk_ref) {
        EXPECT_CALL(*m_sample_agg, sample_region(M_CLK_REF_IDX, rid.first))
            .WillRepeatedly(Return(rid.second));
    }

    // same hint values for all regions
    EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_COMPUTE_IDX, _))
        .WillRepeatedly(Return(0.2));
    EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_MEMORY_IDX, _))
        .WillRepeatedly(Return(0.3));
    EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_IO_IDX, _))
        .WillRepeatedly(Return(0.4));
    EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_SERIAL_IDX, _))
        .WillRepeatedly(Return(0.5));
    EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_PARALLEL_IDX, _))
        .WillRepeatedly(Return(0.6));
    EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_UNKNOWN_IDX, _))
        .WillRepeatedly(Return(0.7));
    EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_UNSET_IDX, _))
        .WillRepeatedly(Return(0.8));
    EXPECT_CALL(*m_sample_agg, sample_region(M_TIME_SPIN_IDX, _))
        .WillRepeatedly(Return(0.9));

    // Other calls
    EXPECT_CALL(m_tree_comm, overhead_send()).WillOnce(Return(678 * 56));
    EXPECT_CALL(*m_comm, rank()).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_comm, num_rank()).WillOnce(Return(1));
}

TEST_F(ReporterTest, generate)
{
    std::set<std::string> signal_names = {};
    EXPECT_CALL(m_platform_io, signal_names()).WillOnce(Return(signal_names));

    //setup default values for 'generate' tests
    generate_setup();

    const std::vector<std::pair<std::string, int> > env_signals = {
        {"CPU_ENERGY", geopm_domain_e::GEOPM_DOMAIN_PACKAGE}
    };
    m_reporter = geopm::make_unique<ReporterImp>(m_start_time,
                                                 m_report_name,
                                                 m_platform_io,
                                                 m_platform_topo,
                                                 0,
                                                 m_sample_agg,
                                                 m_region_agg,
                                                 env_signals,
                                                 "",
                                                 true,
                                                 true);
    m_reporter->init();

    std::vector<std::pair<std::string, std::string> > agent_header {
        {"one", "1"},
        {"two", "2"} };
    std::vector<std::pair<std::string, std::string> > agent_node_report {
        {"three", "3"},
        {"four", "4"} };

    std::string expected = "GEOPM Version: " + std::string(geopm_version()) + "\n"
        "Start Time: " + m_start_time + "\n"
        "Profile: " + m_profile_name + "\n"
        "Agent: my_agent\n"
        "Policy: DYNAMIC\n"
        "one: 1\n"
        "two: 2\n"
        "\n"
        "Hosts:\n"
        "  " + geopm::hostname() + ":\n"
        "    three: 3\n"
        "    four: 4\n"
        "    Regions:\n"
        "    -\n"
        "      region: \"all2all\"\n"
        "      hash: 0x3ddc81bf\n"
        "      runtime (s): 33.33\n"
        "      count: 20\n"
        "      sync-runtime (s): 555\n"
        "      package-energy (J): 388.5\n"
        "      dram-energy (J): 388.5\n"
        "      power (W): 0.7\n"
        "      frequency (%): 81.8182\n"
        "      frequency (Hz): 0.818182\n"
        "      time-hint-network (s): 3.4\n"
        "      time-hint-ignore (s): 3.5\n"
        "      time-hint-compute (s): 0.2\n"
        "      time-hint-memory (s): 0.3\n"
        "      time-hint-io (s): 0.4\n"
        "      time-hint-serial (s): 0.5\n"
        "      time-hint-parallel (s): 0.6\n"
        "      time-hint-unknown (s): 0.7\n"
        "      time-hint-unset (s): 0.8\n"
        "      time-hint-spin (s): 0.9\n"
        "      CPU_ENERGY@package-0: 194.25\n"
        "      CPU_ENERGY@package-1: 194.25\n"
        "      agent stat: 1\n"
        "      agent other stat: 2\n"
        "    -\n"
        "      region: \"model-init\"\n"
        "      hash: 0x644f9787\n"
        "      runtime (s): 22.11\n"
        "      count: 1\n"
        "      sync-runtime (s): 333\n"
        "      package-energy (J): 444\n"
        "      dram-energy (J): 444\n"
        "      power (W): 1.33333\n"
        "      frequency (%): 84.8485\n"
        "      frequency (Hz): 0.848485\n"
        "      time-hint-network (s): 5.6\n"
        "      time-hint-ignore (s): 5.7\n"
        "      time-hint-compute (s): 0.2\n"
        "      time-hint-memory (s): 0.3\n"
        "      time-hint-io (s): 0.4\n"
        "      time-hint-serial (s): 0.5\n"
        "      time-hint-parallel (s): 0.6\n"
        "      time-hint-unknown (s): 0.7\n"
        "      time-hint-unset (s): 0.8\n"
        "      time-hint-spin (s): 0.9\n"
        "      CPU_ENERGY@package-0: 222\n"
        "      CPU_ENERGY@package-1: 222\n"
        "      agent stat: 2\n"
        "    Unmarked Totals:\n"
        "      runtime (s): 0.56\n"
        "      count: 0\n"
        "      sync-runtime (s): 444\n"
        "      package-energy (J): 111\n"
        "      dram-energy (J): 111\n"
        "      power (W): 0.25\n"
        "      frequency (%): 77.2727\n"
        "      frequency (Hz): 0.772727\n"
        "      time-hint-network (s): 1.2\n"
        "      time-hint-ignore (s): 1.3\n"
        "      time-hint-compute (s): 0.2\n"
        "      time-hint-memory (s): 0.3\n"
        "      time-hint-io (s): 0.4\n"
        "      time-hint-serial (s): 0.5\n"
        "      time-hint-parallel (s): 0.6\n"
        "      time-hint-unknown (s): 0.7\n"
        "      time-hint-unset (s): 0.8\n"
        "      time-hint-spin (s): 0.9\n"
        "      CPU_ENERGY@package-0: 55.5\n"
        "      CPU_ENERGY@package-1: 55.5\n"
        "      agent stat: 3\n"
        "    Epoch Totals:\n"
        "      runtime (s): 70\n"
        "      count: 66\n"
        "      sync-runtime (s): 70\n"
        "      package-energy (J): 167\n"
        "      dram-energy (J): 167\n"
        "      power (W): 2.38571\n"
        "      frequency (%): 88.6364\n"
        "      frequency (Hz): 0.886364\n"
        "      time-hint-network (s): 4.2\n"
        "      time-hint-ignore (s): 4.3\n"
        "      time-hint-compute (s): 0.2\n"
        "      time-hint-memory (s): 0.3\n"
        "      time-hint-io (s): 0.4\n"
        "      time-hint-serial (s): 0.5\n"
        "      time-hint-parallel (s): 0.6\n"
        "      time-hint-unknown (s): 0.7\n"
        "      time-hint-unset (s): 0.8\n"
        "      time-hint-spin (s): 0.9\n"
        "      CPU_ENERGY@package-0: 83.5\n"
        "      CPU_ENERGY@package-1: 83.5\n"
        "    Application Totals:\n"
        "      runtime (s): 56\n"
        "      count: 0\n"
        "      sync-runtime (s): 56\n"
        "      package-energy (J): 2222\n"
        "      dram-energy (J): 2222\n"
        "      power (W): 39.6786\n"
        "      frequency (%): 66.6447\n"
        "      frequency (Hz): 0.666447\n"
        "      time-hint-network (s): 45\n"
        "      time-hint-ignore (s): 46\n"
        "      time-hint-compute (s): 0.2\n"
        "      time-hint-memory (s): 0.3\n"
        "      time-hint-io (s): 0.4\n"
        "      time-hint-serial (s): 0.5\n"
        "      time-hint-parallel (s): 0.6\n"
        "      time-hint-unknown (s): 0.7\n"
        "      time-hint-unset (s): 0.8\n"
        "      time-hint-spin (s): 0.9\n"
        "      CPU_ENERGY@package-0: 1111\n"
        "      CPU_ENERGY@package-1: 1111\n"
        "      GEOPM overhead (s): 0.123\n"
        "      GEOPM startup (s): 0.321\n"
        "      geopmctl memory HWM (B): @ANY_STRING@\n"
        "      geopmctl network BW (B/s): 678\n\n";

    std::istringstream exp_stream(expected);

    m_reporter->update();
    m_reporter->total_time(56.0);
    m_reporter->overhead(0.123, 0.321);
    m_reporter->generate("my_agent", agent_header, agent_node_report, m_region_agent_detail,
                         m_application_io,
                         m_comm, m_tree_comm);
    std::ifstream report(m_report_name);
    check_report(exp_stream, report);
}

TEST_F(ReporterTest, generate_conditional)
{

    // GPU signals
    EXPECT_CALL(*m_sample_agg, push_signal("GPU_ENERGY", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(M_ENERGY_GPU_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("GPU_POWER", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(M_POWER_GPU_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(M_FREQUENCY_GPU_IDX));
    EXPECT_CALL(*m_sample_agg, push_signal("CPU_UNCORE_FREQUENCY_STATUS", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(M_FREQUENCY_CPU_UNCORE_IDX));

    std::set<std::string> signal_names = {"GPU_ENERGY","GPU_POWER","GPU_CORE_FREQUENCY_STATUS","CPU_UNCORE_FREQUENCY_STATUS"};
    EXPECT_CALL(m_platform_io, signal_names()).WillOnce(Return(signal_names));

    //setup default values for 'generate' tests
    generate_setup();

    const std::vector<std::pair<std::string, int> > env_signals = {
        {"CPU_ENERGY", geopm_domain_e::GEOPM_DOMAIN_PACKAGE}
    };
    m_reporter = geopm::make_unique<ReporterImp>(m_start_time,
                                                 m_report_name,
                                                 m_platform_io,
                                                 m_platform_topo,
                                                 0,
                                                 m_sample_agg,
                                                 m_region_agg,
                                                 env_signals,
                                                 "",
                                                 true,
                                                 true);
    m_reporter->init();
    m_reporter->total_time(56.0);

    for (auto rid : m_region_energy) {
        EXPECT_CALL(*m_sample_agg, sample_region(M_ENERGY_GPU_IDX, rid.first))
            .WillRepeatedly(Return(rid.second/1.0));
    }

    for (auto rid : m_region_frequency_cpu_uncore) {
        EXPECT_CALL(*m_sample_agg, sample_region(M_FREQUENCY_CPU_UNCORE_IDX, rid.first))
            .WillRepeatedly(Return(rid.second/1.0));
    }
    for (auto rid : m_region_frequency_gpu) {
        EXPECT_CALL(*m_sample_agg, sample_region(M_FREQUENCY_GPU_IDX, rid.first))
            .WillRepeatedly(Return(rid.second/1.0));
    }

    for (auto rid : m_region_power_gpu) {
        EXPECT_CALL(*m_sample_agg, sample_region(M_POWER_GPU_IDX, rid.first))
            .WillRepeatedly(Return(rid.second/1.0));
    }

    std::vector<std::pair<std::string, std::string> > agent_header {
        {"one", "1"},
        {"two", "2"} };
    std::vector<std::pair<std::string, std::string> > agent_node_report {
        {"three", "3"},
        {"four", "4"} };

    std::string expected = "GEOPM Version: " + std::string(geopm_version()) + "\n"
        "Start Time: " + m_start_time + "\n"
        "Profile: " + m_profile_name + "\n"
        "Agent: my_agent\n"
        "Policy: DYNAMIC\n"
        "one: 1\n"
        "two: 2\n"
        "\n"
        "Hosts:\n"
        "  " + geopm::hostname() + ":\n"
        "    three: 3\n"
        "    four: 4\n"
        "    Regions:\n"
        "    -\n"
        "      region: \"all2all\"\n"
        "      hash: 0x3ddc81bf\n"
        "      runtime (s): 33.33\n"
        "      count: 20\n"
        "      sync-runtime (s): 555\n"
        "      package-energy (J): 388.5\n"
        "      dram-energy (J): 388.5\n"
        "      power (W): 0.7\n"
        "      frequency (%): 81.8182\n"
        "      frequency (Hz): 0.818182\n"
        "      time-hint-network (s): 3.4\n"
        "      time-hint-ignore (s): 3.5\n"
        "      time-hint-compute (s): 0.2\n"
        "      time-hint-memory (s): 0.3\n"
        "      time-hint-io (s): 0.4\n"
        "      time-hint-serial (s): 0.5\n"
        "      time-hint-parallel (s): 0.6\n"
        "      time-hint-unknown (s): 0.7\n"
        "      time-hint-unset (s): 0.8\n"
        "      time-hint-spin (s): 0.9\n"
        "      gpu-energy (J): 777\n"
        "      gpu-power (W): 764\n"
        "      gpu-frequency (Hz): 567\n"
        "      uncore-frequency (Hz): 755\n"
        "      CPU_ENERGY@package-0: 194.25\n"
        "      CPU_ENERGY@package-1: 194.25\n"
        "      agent stat: 1\n"
        "      agent other stat: 2\n"
        "    -\n"
        "      region: \"model-init\"\n"
        "      hash: 0x644f9787\n"
        "      runtime (s): 22.11\n"
        "      count: 1\n"
        "      sync-runtime (s): 333\n"
        "      package-energy (J): 444\n"
        "      dram-energy (J): 444\n"
        "      power (W): 1.33333\n"
        "      frequency (%): 84.8485\n"
        "      frequency (Hz): 0.848485\n"
        "      time-hint-network (s): 5.6\n"
        "      time-hint-ignore (s): 5.7\n"
        "      time-hint-compute (s): 0.2\n"
        "      time-hint-memory (s): 0.3\n"
        "      time-hint-io (s): 0.4\n"
        "      time-hint-serial (s): 0.5\n"
        "      time-hint-parallel (s): 0.6\n"
        "      time-hint-unknown (s): 0.7\n"
        "      time-hint-unset (s): 0.8\n"
        "      time-hint-spin (s): 0.9\n"
        "      gpu-energy (J): 888\n"
        "      gpu-power (W): 653\n"
        "      gpu-frequency (Hz): 890\n"
        "      uncore-frequency (Hz): 198\n"
        "      CPU_ENERGY@package-0: 222\n"
        "      CPU_ENERGY@package-1: 222\n"
        "      agent stat: 2\n"
        "    Unmarked Totals:\n"
        "      runtime (s): 0.56\n"
        "      count: 0\n"
        "      sync-runtime (s): 444\n"
        "      package-energy (J): 111\n"
        "      dram-energy (J): 111\n"
        "      power (W): 0.25\n"
        "      frequency (%): 77.2727\n"
        "      frequency (Hz): 0.772727\n"
        "      time-hint-network (s): 1.2\n"
        "      time-hint-ignore (s): 1.3\n"
        "      time-hint-compute (s): 0.2\n"
        "      time-hint-memory (s): 0.3\n"
        "      time-hint-io (s): 0.4\n"
        "      time-hint-serial (s): 0.5\n"
        "      time-hint-parallel (s): 0.6\n"
        "      time-hint-unknown (s): 0.7\n"
        "      time-hint-unset (s): 0.8\n"
        "      time-hint-spin (s): 0.9\n"
        "      gpu-energy (J): 222\n"
        "      gpu-power (W): 211\n"
        "      gpu-frequency (Hz): 123\n"
        "      uncore-frequency (Hz): 421\n"
        "      CPU_ENERGY@package-0: 55.5\n"
        "      CPU_ENERGY@package-1: 55.5\n"
        "      agent stat: 3\n"
        "    Epoch Totals:\n"
        "      runtime (s): 70\n"
        "      count: 66\n"
        "      sync-runtime (s): 70\n"
        "      package-energy (J): 167\n"
        "      dram-energy (J): 167\n"
        "      power (W): 2.38571\n"
        "      frequency (%): 88.6364\n"
        "      frequency (Hz): 0.886364\n"
        "      time-hint-network (s): 4.2\n"
        "      time-hint-ignore (s): 4.3\n"
        "      time-hint-compute (s): 0.2\n"
        "      time-hint-memory (s): 0.3\n"
        "      time-hint-io (s): 0.4\n"
        "      time-hint-serial (s): 0.5\n"
        "      time-hint-parallel (s): 0.6\n"
        "      time-hint-unknown (s): 0.7\n"
        "      time-hint-unset (s): 0.8\n"
        "      time-hint-spin (s): 0.9\n"
        "      gpu-energy (J): 334\n"
        "      gpu-power (W): 432\n"
        "      gpu-frequency (Hz): 456\n"
        "      uncore-frequency (Hz): 653\n"
        "      CPU_ENERGY@package-0: 83.5\n"
        "      CPU_ENERGY@package-1: 83.5\n"
        "    Application Totals:\n"
        "      runtime (s): 56\n"
        "      count: 0\n"
        "      sync-runtime (s): 56\n"
        "      package-energy (J): 2222\n"
        "      dram-energy (J): 2222\n"
        "      power (W): 39.6786\n"
        "      frequency (%): 66.6447\n"
        "      frequency (Hz): 0.666447\n"
        "      time-hint-network (s): 45\n"
        "      time-hint-ignore (s): 46\n"
        "      time-hint-compute (s): 0.2\n"
        "      time-hint-memory (s): 0.3\n"
        "      time-hint-io (s): 0.4\n"
        "      time-hint-serial (s): 0.5\n"
        "      time-hint-parallel (s): 0.6\n"
        "      time-hint-unknown (s): 0.7\n"
        "      time-hint-unset (s): 0.8\n"
        "      time-hint-spin (s): 0.9\n"
        "      gpu-energy (J): 4444\n"
        "      gpu-power (W): 8992\n"
        "      gpu-frequency (Hz): 74489\n"
        "      uncore-frequency (Hz): 121213\n"
        "      CPU_ENERGY@package-0: 1111\n"
        "      CPU_ENERGY@package-1: 1111\n"
        "      GEOPM overhead (s): 0.123\n"
        "      GEOPM startup (s): 0.321\n"
        "      geopmctl memory HWM (B): @ANY_STRING@\n"
        "      geopmctl network BW (B/s): 678\n\n";

    std::istringstream exp_stream(expected);

    m_reporter->update();
    m_reporter->overhead(0.123, 0.321);
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
        std::string exp_str = exp_line;
        size_t pos = exp_str.find("@ANY_STRING@");
        if (pos == exp_str.npos) {
            ASSERT_STREQ(exp_line, res_line) << " on line " << line;
        }
        else {
            exp_str = exp_str.substr(0, pos);
            ASSERT_THAT(std::string(res_line), HasSubstr(exp_str)) << " on line " << line;
        }
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

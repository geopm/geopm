/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <limits.h>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <libgen.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "contrib/json11/json11.hpp"

#include "geopm_sched.h"
#include "geopm_hash.h"
#include "Helper.hpp"
#include "PlatformTopo.hpp"
#include "MSRIOImp.hpp"
#include "MSR.hpp"
#include "Exception.hpp"
#include "PluginFactory.hpp"
#include "MSRIOGroup.hpp"
#include "MockPlatformTopo.hpp"
#include "MockMSRIO.hpp"
#include "geopm_test.hpp"

using geopm::MSRIOGroup;
using geopm::PlatformTopo;
using geopm::Exception;
using geopm::MSR;
using testing::Return;
using testing::SetArgReferee;
using testing::_;
using testing::WithArg;
using testing::AtLeast;
using json11::Json;

class MSRIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        std::vector<std::string> m_test_dev_path;
        std::unique_ptr<MSRIOGroup> m_msrio_group;
        std::shared_ptr<MockPlatformTopo> m_topo;
        std::shared_ptr<MockMSRIO> m_msrio;
        int m_num_package = 2;
        int m_num_core = 4;
        int m_num_cpu = 16;
        void mock_enable_fixed_counters(void);
};

void MSRIOGroupTest::SetUp()
{
    m_topo = make_topo(m_num_package, m_num_core, m_num_cpu);
    m_msrio = std::make_shared<MockMSRIO>();

    // suppress warnings about num_domain and domain_nested calls
    EXPECT_CALL(*m_topo, num_domain(_)).Times(AtLeast(0));
    EXPECT_CALL(*m_topo, domain_nested(_, _, _)).Times(AtLeast(0));
    // suppress mock calls from initalizing counter enables
    EXPECT_CALL(*m_msrio, write_msr(_, _, _, _)).Times(AtLeast(0));
    m_msrio_group = geopm::make_unique<MSRIOGroup>(*m_topo, m_msrio,
                                                   MSRIOGroup::M_CPUID_SKX,
                                                   m_num_cpu);
}

TEST_F(MSRIOGroupTest, supported_cpuid)
{
    // Check that MSRIOGroup can be safely constructed for supported platforms
    std::vector<uint64_t> cpuids = {
        MSRIOGroup::M_CPUID_SNB,
        MSRIOGroup::M_CPUID_IVT,
        MSRIOGroup::M_CPUID_HSX,
        MSRIOGroup::M_CPUID_BDX,
        MSRIOGroup::M_CPUID_KNL,
        MSRIOGroup::M_CPUID_SKX,
        MSRIOGroup::M_CPUID_ICX,
    };
    for (auto id : cpuids) {
        try {
            MSRIOGroup(*m_topo, m_msrio, id, m_num_cpu);
        }
        catch (const std::exception &ex) {
            FAIL() << "Could not construct MSRIOGroup for cpuid 0x"
                   << std::hex << id << std::dec << ": " << ex.what();
        }
    }

    // unsupported cpuid
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup(*m_topo, m_msrio, 0x9999, m_num_cpu),
                               GEOPM_ERROR_RUNTIME, "Unsupported CPUID");
}

TEST_F(MSRIOGroupTest, valid_signal_names)
{
    std::vector<std::string> signal_aliases;

    //// energy signals
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::PKG_ENERGY_STATUS:ENERGY"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::DRAM_ENERGY_STATUS:ENERGY"));
    signal_aliases.push_back("ENERGY_PACKAGE");
    signal_aliases.push_back("ENERGY_DRAM");

    //// counters
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::FIXED_CTR1:CPU_CLK_UNHALTED_THREAD"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::FIXED_CTR2:CPU_CLK_UNHALTED_REF_TSC"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::TIME_STAMP_COUNTER:TIMESTAMP_COUNT"));
    signal_aliases.push_back("INSTRUCTIONS_RETIRED");
    signal_aliases.push_back("CYCLES_THREAD");
    signal_aliases.push_back("CYCLES_REFERENCE");
    signal_aliases.push_back("TIMESTAMP_COUNTER");

    //// frequency signals
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::PERF_STATUS:FREQ"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0"));
    signal_aliases.push_back("FREQUENCY");
    signal_aliases.push_back("FREQUENCY_MAX");
    // note: FREQUENCY_MIN and FREQUENCY_STICKER come from CpuinfoIOGroup.

    //// temperature signals
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::TEMPERATURE_TARGET:PROCHOT_MIN"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::THERM_STATUS:DIGITAL_READOUT"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::PACKAGE_THERM_STATUS:DIGITAL_READOUT"));
    signal_aliases.push_back("TEMPERATURE_CORE");
    signal_aliases.push_back("TEMPERATURE_PACKAGE");

    //// power signals
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::PKG_POWER_INFO:MIN_POWER"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::PKG_POWER_INFO:MAX_POWER"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER"));
    signal_aliases.push_back("POWER_PACKAGE_MIN");
    signal_aliases.push_back("POWER_PACKAGE_MAX");
    signal_aliases.push_back("POWER_PACKAGE_TDP");
    signal_aliases.push_back("POWER_PACKAGE");
    signal_aliases.push_back("POWER_DRAM");

    auto signal_names = m_msrio_group->signal_names();
    for (const auto &name : signal_aliases) {
        // check names appear in signal_names
        EXPECT_TRUE(signal_names.find(name) != signal_names.end()) << name;
    }
    for (const auto &name : signal_names) {
        // check signal names are valid
        EXPECT_TRUE(m_msrio_group->is_valid_signal(name)) << name;
        // check that there is some non-empty description
        EXPECT_FALSE(m_msrio_group->signal_description(name).empty()) << name;
        // check that signals have a valid behavior enum
        EXPECT_LT(-1, m_msrio_group->signal_behavior(name)) << name;
    }
}

TEST_F(MSRIOGroupTest, valid_signal_domains)
{
    // energy
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE, m_msrio_group->signal_domain_type("ENERGY_PACKAGE"));
    EXPECT_EQ(GEOPM_DOMAIN_BOARD_MEMORY, m_msrio_group->signal_domain_type("ENERGY_DRAM"));

    // counter
    EXPECT_EQ(GEOPM_DOMAIN_CPU, m_msrio_group->signal_domain_type("INSTRUCTIONS_RETIRED"));
    EXPECT_EQ(GEOPM_DOMAIN_CPU, m_msrio_group->signal_domain_type("CYCLES_THREAD"));
    EXPECT_EQ(GEOPM_DOMAIN_CPU, m_msrio_group->signal_domain_type("CYCLES_REFERENCE"));
    EXPECT_EQ(GEOPM_DOMAIN_CPU, m_msrio_group->signal_domain_type("TIMESTAMP_COUNTER"));

    // frequency
    EXPECT_EQ(GEOPM_DOMAIN_CPU, m_msrio_group->signal_domain_type("FREQUENCY"));
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE, m_msrio_group->signal_domain_type("FREQUENCY_MAX"));

    // temperature
    EXPECT_EQ(GEOPM_DOMAIN_CORE,
              m_msrio_group->signal_domain_type("TEMPERATURE_CORE"));
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE,
              m_msrio_group->signal_domain_type("TEMPERATURE_PACKAGE"));

    // power
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE,
              m_msrio_group->signal_domain_type("POWER_PACKAGE_MIN"));
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE,
              m_msrio_group->signal_domain_type("POWER_PACKAGE_MAX"));
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE,
              m_msrio_group->signal_domain_type("POWER_PACKAGE_TDP"));
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE,
              m_msrio_group->signal_domain_type("POWER_PACKAGE"));
    EXPECT_EQ(GEOPM_DOMAIN_BOARD_MEMORY,
              m_msrio_group->signal_domain_type("POWER_DRAM"));
}

TEST_F(MSRIOGroupTest, valid_signal_aggregation)
{
    std::function<double(const std::vector<double> &)> func;

    // energy
    func = m_msrio_group->agg_function("ENERGY_PACKAGE");
    EXPECT_TRUE(is_agg_sum(func));
    func = m_msrio_group->agg_function("ENERGY_DRAM");
    EXPECT_TRUE(is_agg_sum(func));

    // counter
    func = m_msrio_group->agg_function("INSTRUCTIONS_RETIRED");
    EXPECT_TRUE(is_agg_sum(func));
    func = m_msrio_group->agg_function("CYCLES_THREAD");
    EXPECT_TRUE(is_agg_sum(func));
    func = m_msrio_group->agg_function("CYCLES_REFERENCE");
    EXPECT_TRUE(is_agg_sum(func));
    /// @todo: what should this be?
    //func = m_msrio_group->agg_function("TIMESTAMP_COUNTER");
    //EXPECT_TRUE(is_agg_sum(func));

    // frequency
    func = m_msrio_group->agg_function("FREQUENCY");
    EXPECT_TRUE(is_agg_average(func));
    /// @todo: what should this be?
    //func = m_msrio_group->agg_function("FREQUENCY_MAX");
    //EXPECT_TRUE(is_agg_expect_same(func));

    // temperature
    func = m_msrio_group->agg_function("TEMPERATURE_CORE");
    EXPECT_TRUE(is_agg_average(func));
    func = m_msrio_group->agg_function("TEMPERATURE_PACKAGE");
    EXPECT_TRUE(is_agg_average(func));

    // power
    // @todo: POWER_PACKAGE and POWER_DRAM

    // @todo: what should this be?
    //func = m_msrio_group->agg_function("POWER_PACKAGE_MIN");
    //EXPECT_TRUE(is_agg_expect_same(func));
    //func = m_msrio_group->agg_function("POWER_PACKAGE_MAX");
    //EXPECT_TRUE(is_agg_expect_same(func));
    //func = m_msrio_group->agg_function("POWER_PACKAGE_TDP");
    //EXPECT_TRUE(is_agg_expect_same(func));
    func = m_msrio_group->agg_function("POWER_PACKAGE");
    EXPECT_TRUE(is_agg_sum(func));
    func = m_msrio_group->agg_function("POWER_DRAM");
    EXPECT_TRUE(is_agg_sum(func));
}

TEST_F(MSRIOGroupTest, valid_signal_format)
{
    std::function<std::string(double)> func;

    // most SI signals are printed as double
    std::vector<std::string> si_alias = {
        "ENERGY_PACKAGE", "ENERGY_DRAM",
        "FREQUENCY", "FREQUENCY_MAX",
        "TEMPERATURE_CORE", "TEMPERATURE_PACKAGE",
        "POWER_PACKAGE_MIN", "POWER_PACKAGE_MAX", "POWER_PACKAGE_TDP",
        "POWER_PACKAGE", "POWER_DRAM"
    };
    for (const auto &name : si_alias) {
        func = m_msrio_group->format_function(name);
        EXPECT_TRUE(is_format_double(func));
    }

    // counter - no units, printed as integer
    std::vector<std::string> count_alias = {
        "INSTRUCTIONS_RETIRED",
        "CYCLES_THREAD",
        "CYCLES_REFERENCE"
    };
    for (const auto &name : count_alias) {
        func = m_msrio_group->format_function(name);
        EXPECT_TRUE(is_format_integer(func));
    }

    // raw MSRs printed in hex
    func = m_msrio_group->format_function("MSR::PERF_STATUS#");
    EXPECT_TRUE(is_format_raw64(func));

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->format_function("INVALID"),
                               GEOPM_ERROR_INVALID, "not valid for MSRIOGroup");
}

TEST_F(MSRIOGroupTest, signal_error)
{
    // error cases for push_signal
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 9000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    // sample
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->sample(-1), GEOPM_ERROR_INVALID, "signal_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->sample(22), GEOPM_ERROR_INVALID, "signal_idx out of range");

    // read_signal
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->read_signal("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->read_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->read_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 9000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
}

TEST_F(MSRIOGroupTest, push_signal)
{
    EXPECT_TRUE(m_msrio_group->is_valid_signal("MSR::PERF_STATUS:FREQ"));
    EXPECT_FALSE(m_msrio_group->is_valid_signal("INVALID"));
    EXPECT_EQ(GEOPM_DOMAIN_CPU, m_msrio_group->signal_domain_type("MSR::FIXED_CTR0:INST_RETIRED_ANY"));
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_msrio_group->signal_domain_type("INVALID"));

    // index to memory location inside of MSRIO
    enum msrio_idx_e {
        PERF_STATUS_0,
        INST_RET_0,
        INST_RET_1
    };
    uint64_t perf_status_offset = 0x198;
    uint64_t inst_ret_offset = 0x309;
    EXPECT_CALL(*m_msrio, add_read(0, perf_status_offset))
        .WillOnce(Return(PERF_STATUS_0));
    EXPECT_CALL(*m_msrio, add_read(0, inst_ret_offset))
        .WillOnce(Return(INST_RET_0));
    EXPECT_CALL(*m_msrio, add_read(1, inst_ret_offset))
        .WillOnce(Return(INST_RET_1));

    // push valid signals
    int freq_idx_0 = m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 0);
    ASSERT_EQ(0, freq_idx_0);
    int inst_idx_0 = m_msrio_group->push_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY",
                                                GEOPM_DOMAIN_CPU, 0);
    ASSERT_EQ(1, inst_idx_0);

    // pushing same signal gives same index
    int idx2 = m_msrio_group->push_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(inst_idx_0, idx2);

    // pushing signal alias gives same index
    int idx3 = m_msrio_group->push_signal("INSTRUCTIONS_RETIRED", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(inst_idx_0, idx3);

    // pushing same signal for another cpu gives different index
    int inst_idx_1 = m_msrio_group->push_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY", GEOPM_DOMAIN_CPU, 1);
    EXPECT_NE(inst_idx_0, inst_idx_1);

    // all provided signals are valid
    EXPECT_NE(0u, m_msrio_group->signal_names().size());
    for (const auto &sig : m_msrio_group->signal_names()) {
        EXPECT_TRUE(m_msrio_group->is_valid_signal(sig));
    }
}

TEST_F(MSRIOGroupTest, sample)
{
    // index to memory location inside of MSRIO
    enum msrio_idx_e {
        PERF_STATUS_0,
        INST_RET_0,
        INST_RET_1
    };
    uint64_t perf_status_offset = 0x198;
    uint64_t inst_ret_offset = 0x309;
    EXPECT_CALL(*m_msrio, add_read(0, perf_status_offset))
        .WillOnce(Return(PERF_STATUS_0));
    EXPECT_CALL(*m_msrio, add_read(0, inst_ret_offset))
        .WillOnce(Return(INST_RET_0));
    EXPECT_CALL(*m_msrio, add_read(1, inst_ret_offset))
        .WillOnce(Return(INST_RET_1));
    int freq_idx_0 = m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 0);
    int inst_idx_0 = m_msrio_group->push_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY",
                                                GEOPM_DOMAIN_CPU, 0);
    int inst_idx_1 = m_msrio_group->push_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY",
                                                GEOPM_DOMAIN_CPU, 1);
    EXPECT_NE(freq_idx_0, inst_idx_0);
    EXPECT_NE(freq_idx_0, inst_idx_1);
    EXPECT_NE(inst_idx_0, inst_idx_1);

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->sample(freq_idx_0),
                               GEOPM_ERROR_RUNTIME, "sample() called before signal was read");

    // first batch
    {
    EXPECT_CALL(*m_msrio, read_batch());
    m_msrio_group->read_batch();

    EXPECT_CALL(*m_msrio, sample(PERF_STATUS_0)).WillOnce(Return(0xB00));
    EXPECT_CALL(*m_msrio, sample(INST_RET_0)).WillOnce(Return(1234));
    EXPECT_CALL(*m_msrio, sample(INST_RET_1)).WillOnce(Return(5678));
    double freq_0 = m_msrio_group->sample(freq_idx_0);
    double inst_0 = m_msrio_group->sample(inst_idx_0);
    double inst_1 = m_msrio_group->sample(inst_idx_1);
    EXPECT_EQ(1.1e9, freq_0);
    EXPECT_EQ(1234, inst_0);
    EXPECT_EQ(5678, inst_1);
    }

    // sample again without read should get same value
    {
    EXPECT_CALL(*m_msrio, sample(PERF_STATUS_0)).WillOnce(Return(0xB00));
    EXPECT_CALL(*m_msrio, sample(INST_RET_0)).WillOnce(Return(1234));
    EXPECT_CALL(*m_msrio, sample(INST_RET_1)).WillOnce(Return(5678));
    double freq_0 = m_msrio_group->sample(freq_idx_0);
    double inst_0 = m_msrio_group->sample(inst_idx_0);
    double inst_1 = m_msrio_group->sample(inst_idx_1);
    EXPECT_EQ(1.1e9, freq_0);
    EXPECT_EQ(1234, inst_0);
    EXPECT_EQ(5678, inst_1);
    }

    // second batch
    {
    EXPECT_CALL(*m_msrio, read_batch());
    m_msrio_group->read_batch();

    EXPECT_CALL(*m_msrio, sample(PERF_STATUS_0)).WillOnce(Return(0xC00));
    EXPECT_CALL(*m_msrio, sample(INST_RET_0)).WillOnce(Return(87654));
    EXPECT_CALL(*m_msrio, sample(INST_RET_1)).WillOnce(Return(65432));
    double freq_0 = m_msrio_group->sample(freq_idx_0);
    double inst_0 = m_msrio_group->sample(inst_idx_0);
    double inst_1 = m_msrio_group->sample(inst_idx_1);
    EXPECT_EQ(1.2e9, freq_0);
    EXPECT_EQ(87654, inst_0);
    EXPECT_EQ(65432, inst_1);
    }

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "cannot push a signal after read_batch");
}

TEST_F(MSRIOGroupTest, sample_raw)
{
    uint64_t fixed_ctr_offset = 0x309;
    EXPECT_CALL(*m_msrio, add_read(0, fixed_ctr_offset)).WillOnce(Return(0));
    EXPECT_CALL(*m_msrio, add_read(1, fixed_ctr_offset)).WillOnce(Return(1));
    int inst_idx_0 = m_msrio_group->push_signal("MSR::FIXED_CTR0#",
                                                GEOPM_DOMAIN_CPU, 0);
    int inst_idx_1 = m_msrio_group->push_signal("MSR::FIXED_CTR0#",
                                                GEOPM_DOMAIN_CPU, 1);

    EXPECT_CALL(*m_msrio, read_batch());
    m_msrio_group->read_batch();

    EXPECT_CALL(*m_msrio, sample(0)).WillOnce(Return(0xB000D000F0001234));
    EXPECT_CALL(*m_msrio, sample(1)).WillOnce(Return(0xB000D000F0001235));
    uint64_t inst_0 = geopm_signal_to_field(m_msrio_group->sample(inst_idx_0));
    uint64_t inst_1 = geopm_signal_to_field(m_msrio_group->sample(inst_idx_1));
    EXPECT_EQ(0xB000D000F0001234, inst_0);
    EXPECT_EQ(0xB000D000F0001235, inst_1);
}

TEST_F(MSRIOGroupTest, read_signal_energy)
{
    uint64_t pkg_energy_offset = 0x611;
    uint64_t dram_energy_offset = 0x619;
    uint64_t value = 0;
    double result;

    value = 1638400;  // 61uJ units
    EXPECT_CALL(*m_msrio, read_msr(0, pkg_energy_offset)).WillOnce(Return(value));
    result = m_msrio_group->read_signal("ENERGY_PACKAGE", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_NEAR(100, result, 0.0001);

    value = 3276799;  // 15uJ units
    EXPECT_CALL(*m_msrio, read_msr(0, dram_energy_offset)).WillOnce(Return(value));
    result = m_msrio_group->read_signal("ENERGY_DRAM", GEOPM_DOMAIN_BOARD_MEMORY, 0);
    EXPECT_NEAR(50, result, 0.0001);
}

TEST_F(MSRIOGroupTest, read_signal_counter)
{
    uint64_t tsc_offset = 0x10;
    uint64_t fixed0_offset = 0x309;
    uint64_t fixed1_offset = 0x30A;
    uint64_t fixed2_offset = 0x30B;
    double result;

    EXPECT_CALL(*m_msrio, read_msr(0, tsc_offset))
        .WillOnce(Return(11111))
        .WillOnce(Return(22222));
    result = m_msrio_group->read_signal("MSR::TIME_STAMP_COUNTER:TIMESTAMP_COUNT", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(11111, result);
    result = m_msrio_group->read_signal("TIMESTAMP_COUNTER", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(22222, result);

    EXPECT_CALL(*m_msrio, read_msr(0, fixed0_offset))
        .WillOnce(Return(7777))
        .WillOnce(Return(8888));
    result = m_msrio_group->read_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(7777, result);
    result = m_msrio_group->read_signal("INSTRUCTIONS_RETIRED", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(8888, result);

    EXPECT_CALL(*m_msrio, read_msr(0, fixed1_offset))
        .WillOnce(Return(33333))
        .WillOnce(Return(44444));
    result = m_msrio_group->read_signal("MSR::FIXED_CTR1:CPU_CLK_UNHALTED_THREAD", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(33333, result);
    result = m_msrio_group->read_signal("CYCLES_THREAD", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(44444, result);

    EXPECT_CALL(*m_msrio, read_msr(0, fixed2_offset))
        .WillOnce(Return(55555))
        .WillOnce(Return(66666));
    result = m_msrio_group->read_signal("MSR::FIXED_CTR2:CPU_CLK_UNHALTED_REF_TSC", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(55555, result);
    result = m_msrio_group->read_signal("CYCLES_REFERENCE", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(66666, result);
}

TEST_F(MSRIOGroupTest, read_signal_frequency)
{
    uint64_t status_offset = 0x198;
    uint64_t limit_offset = 0x1ad;
    double result;

    EXPECT_CALL(*m_msrio, read_msr(0, status_offset))
        .WillOnce(Return(0xD00))  // 100MHz units, field 15:8
        .WillOnce(Return(0xE00));
    result = m_msrio_group->read_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(1.3e9, result);
    result = m_msrio_group->read_signal("FREQUENCY", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(1.4e9, result);

    // For SKX: MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0 0:7
    EXPECT_CALL(*m_msrio, read_msr(0, limit_offset))
        .WillOnce(Return(0xF));
    result = m_msrio_group->read_signal("FREQUENCY_MAX", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(1.5e9, result);
}

TEST_F(MSRIOGroupTest, read_signal_temperature)
{
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::TEMPERATURE_TARGET:PROCHOT_MIN"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::THERM_STATUS:DIGITAL_READOUT"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::PACKAGE_THERM_STATUS:DIGITAL_READOUT"));

    uint64_t prochot_msr = 0x1A2;
    int prochot_begin = 16;
    int prochot_val = 98;
    uint64_t value = prochot_val << prochot_begin;
    EXPECT_CALL(*m_msrio, read_msr(0, prochot_msr))
        .Times(2) // used by both core and package temperature
        .WillRepeatedly(Return(value));

    uint64_t readout_msr = 0x19C;
    int readout_begin = 16;
    int readout_val = 66;
    value = readout_val << readout_begin;
    EXPECT_CALL(*m_msrio, read_msr(0, readout_msr))
        .WillOnce(Return(value));
    // temperature is (PROCHOT_MIN - DIGITAL_READOUT)
    double exp_temp = prochot_val - readout_val;
    EXPECT_NEAR(exp_temp, m_msrio_group->read_signal("TEMPERATURE_CORE", GEOPM_DOMAIN_CORE, 0), 0.001);

    readout_val = 55;
    exp_temp = prochot_val - readout_val;
    uint64_t pkg_readout_msr = 0x1B1;
    int pkg_readout_begin = 16;
    value = readout_val << pkg_readout_begin;
    EXPECT_CALL(*m_msrio, read_msr(0, pkg_readout_msr))
        .WillOnce(Return(value));
    EXPECT_NEAR(exp_temp, m_msrio_group->read_signal("TEMPERATURE_PACKAGE", GEOPM_DOMAIN_PACKAGE, 0), 0.001);
}

TEST_F(MSRIOGroupTest, read_signal_power)
{
    uint64_t info_offset = 0x614;
    double result;

    // power limits - 1/8W units
    EXPECT_CALL(*m_msrio, read_msr(0, info_offset))
        .WillOnce(Return(0x258))  // TDP in 14:0
        .WillOnce(Return(0x262))
        .WillOnce(Return(0x1920000)) // min in 30:16
        .WillOnce(Return(0x3210000))
        .WillOnce(Return(0x64400000000))  // max in 46:32
        .WillOnce(Return(0x64B00000000));

    result = m_msrio_group->read_signal("MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(75, result);
    result = m_msrio_group->read_signal("POWER_PACKAGE_TDP", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(76.25, result);

    result = m_msrio_group->read_signal("MSR::PKG_POWER_INFO:MIN_POWER", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(50.25, result);
    result = m_msrio_group->read_signal("POWER_PACKAGE_MIN", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(100.125, result);

    result = m_msrio_group->read_signal("MSR::PKG_POWER_INFO:MAX_POWER", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(200.5, result);
    result = m_msrio_group->read_signal("POWER_PACKAGE_MAX", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(201.375, result);
}

TEST_F(MSRIOGroupTest, push_signal_temperature)
{
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::TEMPERATURE_TARGET:PROCHOT_MIN"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::THERM_STATUS:DIGITAL_READOUT"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::PACKAGE_THERM_STATUS:DIGITAL_READOUT"));

    // index to memory location inside of MSRIO
    enum msrio_idx_e {
        PROCHOT_0,
        CORE_READOUT_0,
        PKG_READOUT_0
    };
    uint64_t prochot_msr = 0x1A2;
    uint64_t core_readout_msr = 0x19C;
    uint64_t pkg_readout_msr = 0x1B1;
    EXPECT_CALL(*m_msrio, add_read(0, prochot_msr))
        .WillOnce(Return(PROCHOT_0));
    EXPECT_CALL(*m_msrio, add_read(0, core_readout_msr))
        .WillOnce(Return(CORE_READOUT_0));
    EXPECT_CALL(*m_msrio, add_read(0, pkg_readout_msr))
        .WillOnce(Return(PKG_READOUT_0));

    int core_idx = m_msrio_group->push_signal("TEMPERATURE_CORE", GEOPM_DOMAIN_CORE, 0);
    int pkg_idx = m_msrio_group->push_signal("TEMPERATURE_PACKAGE", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_GE(core_idx, 0);
    EXPECT_GE(pkg_idx, 0);

    EXPECT_CALL(*m_msrio, read_batch());
    m_msrio_group->read_batch();

    int prochot_val = 98;
    int prochot_begin = 16;
    uint64_t value = prochot_val << prochot_begin;
    EXPECT_CALL(*m_msrio, sample(PROCHOT_0)).Times(2)
        .WillRepeatedly(Return(value));

    int readout_val = 66;
    int readout_begin = 16;
    value = readout_val << readout_begin;
    EXPECT_CALL(*m_msrio, sample(CORE_READOUT_0))
        .WillOnce(Return(value));
    // temperature is (PROCHOT_MIN - DIGITAL_READOUT)
    double exp_temp = prochot_val - readout_val;
    EXPECT_NEAR(exp_temp, m_msrio_group->sample(core_idx), 0.001);

    readout_val = 55;
    int pkg_readout_begin = 16;
    value = readout_val << pkg_readout_begin;
    EXPECT_CALL(*m_msrio, sample(PKG_READOUT_0))
        .WillOnce(Return(value));
    exp_temp = prochot_val - readout_val;
    EXPECT_NEAR(exp_temp, m_msrio_group->sample(pkg_idx), 0.001);
}

TEST_F(MSRIOGroupTest, control_error)
{
    // error cases for push_control
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 9000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    // adjust
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->adjust(-1, 0), GEOPM_ERROR_INVALID, "control_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->adjust(22, 0), GEOPM_ERROR_INVALID, "control_idx out of range");

    // write_control
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_control("INVALID", GEOPM_DOMAIN_CPU, 0, 1e9),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, -1, 1e9),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 9000, 1e9),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
}

TEST_F(MSRIOGroupTest, push_control)
{
    EXPECT_TRUE(m_msrio_group->is_valid_control("MSR::PERF_CTL:FREQ"));
    EXPECT_FALSE(m_msrio_group->is_valid_control("INVALID"));
    EXPECT_EQ(GEOPM_DOMAIN_CPU, m_msrio_group->control_domain_type("MSR::FIXED_CTR_CTRL:EN0_OS"));
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_msrio_group->control_domain_type("INVALID"));

    // push valid controls
    uint64_t perf_ctl_offset = 0x199;
    EXPECT_CALL(*m_msrio, add_write(0, perf_ctl_offset));
    EXPECT_CALL(*m_msrio, add_write(4, perf_ctl_offset));
    EXPECT_CALL(*m_msrio, add_write(8, perf_ctl_offset));
    EXPECT_CALL(*m_msrio, add_write(12, perf_ctl_offset));
    int freq_idx_0 = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 0);
    ASSERT_EQ(0, freq_idx_0);
    // pushing same control gives same index
    int idx2 = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 0);
    EXPECT_EQ(freq_idx_0, idx2);

    // pushing alias gives same index
    int idx3 = m_msrio_group->push_control("FREQUENCY", GEOPM_DOMAIN_CORE, 0);
    EXPECT_EQ(freq_idx_0, idx3);

    uint64_t pl1_limit_offset = 0x610;
    EXPECT_CALL(*m_msrio, add_write(0, pl1_limit_offset));
    EXPECT_CALL(*m_msrio, add_write(4, pl1_limit_offset));
    EXPECT_CALL(*m_msrio, add_write(8, pl1_limit_offset));
    EXPECT_CALL(*m_msrio, add_write(12, pl1_limit_offset));
    EXPECT_CALL(*m_msrio, add_write(1, pl1_limit_offset));
    EXPECT_CALL(*m_msrio, add_write(5, pl1_limit_offset));
    EXPECT_CALL(*m_msrio, add_write(9, pl1_limit_offset));
    EXPECT_CALL(*m_msrio, add_write(13, pl1_limit_offset));
    // pushing power limit reads lock bit
    EXPECT_CALL(*m_msrio, read_msr(0, pl1_limit_offset));  // cpu 0 for pkg 0
    EXPECT_CALL(*m_msrio, read_msr(2, pl1_limit_offset));  // cpu 2 for pkg 1
    int power_idx = m_msrio_group->push_control("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT", GEOPM_DOMAIN_PACKAGE, 0);
    ASSERT_EQ(1, power_idx);

    int power_idx1 = m_msrio_group->push_control("POWER_PACKAGE_LIMIT",
                                                 GEOPM_DOMAIN_PACKAGE, 0);
    ASSERT_EQ(power_idx, power_idx1);

    // all provided controls are valid
    EXPECT_NE(0u, m_msrio_group->control_names().size());
    for (const auto &ctl : m_msrio_group->control_names()) {
        EXPECT_TRUE(m_msrio_group->is_valid_control(ctl));
    }
}

TEST_F(MSRIOGroupTest, adjust)
{
    // fake indices for MSRIO
    enum {
        PERF_CTL_0,
        PERF_CTL_1,
        PERF_CTL_2,
        PERF_CTL_3,
        PL1_LIMIT_0,
        PL1_LIMIT_1,
        PL1_LIMIT_2,
        PL1_LIMIT_3,
        PL1_LIMIT_4,
        PL1_LIMIT_5,
        PL1_LIMIT_6,
        PL1_LIMIT_7,
    };

    uint64_t perf_ctl_offset = 0x199;
    EXPECT_CALL(*m_msrio, add_write(0, perf_ctl_offset)).WillOnce(Return(PERF_CTL_0));
    EXPECT_CALL(*m_msrio, add_write(4, perf_ctl_offset)).WillOnce(Return(PERF_CTL_1));
    EXPECT_CALL(*m_msrio, add_write(8, perf_ctl_offset)).WillOnce(Return(PERF_CTL_2));
    EXPECT_CALL(*m_msrio, add_write(12, perf_ctl_offset)).WillOnce(Return(PERF_CTL_3));
    int freq_idx_0 = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 0);
    uint64_t pl1_limit_offset = 0x610;
    EXPECT_CALL(*m_msrio, add_write(0, pl1_limit_offset)).WillOnce(Return(PL1_LIMIT_0));
    EXPECT_CALL(*m_msrio, add_write(4, pl1_limit_offset)).WillOnce(Return(PL1_LIMIT_1));
    EXPECT_CALL(*m_msrio, add_write(8, pl1_limit_offset)).WillOnce(Return(PL1_LIMIT_2));
    EXPECT_CALL(*m_msrio, add_write(12, pl1_limit_offset)).WillOnce(Return(PL1_LIMIT_3));
    EXPECT_CALL(*m_msrio, add_write(1, pl1_limit_offset)).WillOnce(Return(PL1_LIMIT_4));
    EXPECT_CALL(*m_msrio, add_write(5, pl1_limit_offset)).WillOnce(Return(PL1_LIMIT_5));
    EXPECT_CALL(*m_msrio, add_write(9, pl1_limit_offset)).WillOnce(Return(PL1_LIMIT_6));
    EXPECT_CALL(*m_msrio, add_write(13, pl1_limit_offset)).WillOnce(Return(PL1_LIMIT_7));
    // pushing power limit reads lock bit
    // @todo: not getting called??
    //EXPECT_CALL(*m_msrio, read_msr(0, pl1_limit_offset));  // cpu 0 for pkg 0
    //EXPECT_CALL(*m_msrio, read_msr(2, pl1_limit_offset));  // cpu 2 for pkg 1
    int power_idx = m_msrio_group->push_control("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT", GEOPM_DOMAIN_PACKAGE, 0);
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_batch(), GEOPM_ERROR_INVALID,
                               "called before all controls were adjusted");

    uint64_t perf_ctl_mask = 0xFF00;
    uint64_t pl1_limit_mask = 0x7FFF;
    // Set frequency to 1 GHz, power to 100W
    uint64_t encoded_freq = 0xA00ULL;
    uint64_t encoded_power = 0x500ULL;
    {
    // all cpus on core 0
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_0, encoded_freq, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_1, encoded_freq, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_2, encoded_freq, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_3, encoded_freq, perf_ctl_mask));
    // all cpus on package 0
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_0, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_1, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_2, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_3, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_4, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_5, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_6, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_7, encoded_power, pl1_limit_mask));
    m_msrio_group->adjust(freq_idx_0, 1e9);
    m_msrio_group->adjust(power_idx, 160);

    EXPECT_CALL(*m_msrio, write_batch());
    m_msrio_group->write_batch();
    }

    // Calling adjust without calling write_batch() should not
    // change the platform.
    encoded_freq = 0x3200ULL;
    encoded_power = 0x640ULL;
    {
    EXPECT_CALL(*m_msrio, write_batch()).Times(0);

    // all cpus on core 0
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_0, encoded_freq, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_1, encoded_freq, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_2, encoded_freq, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_3, encoded_freq, perf_ctl_mask));
    // all cpus on package 0
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_0, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_1, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_2, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_3, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_4, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_5, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_6, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_7, encoded_power, pl1_limit_mask));

    m_msrio_group->adjust(freq_idx_0, 5e9);
    m_msrio_group->adjust(power_idx, 200);
    }

    // Set frequency to 5 GHz, power to 200W
    {
    // all cpus on core 0
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_0, encoded_freq, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_1, encoded_freq, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_2, encoded_freq, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, adjust(PERF_CTL_3, encoded_freq, perf_ctl_mask));
    // all cpus on package 0
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_0, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_1, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_2, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_3, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_4, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_5, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_6, encoded_power, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, adjust(PL1_LIMIT_7, encoded_power, pl1_limit_mask));
    m_msrio_group->adjust(freq_idx_0, 5e9);
    m_msrio_group->adjust(power_idx, 200);

    EXPECT_CALL(*m_msrio, write_batch());
    m_msrio_group->write_batch();
    }

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "cannot push a control after read_batch() or adjust()");
}

TEST_F(MSRIOGroupTest, write_control)
{
    // Set frequency to 3 GHz immediately
    uint64_t perf_ctl_offset = 0x199;
    uint64_t perf_ctl_mask = 0xFF00;
    // all cpus on core 0
    {
    EXPECT_CALL(*m_msrio, write_msr(0, perf_ctl_offset, 0x1E00ULL, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, write_msr(4, perf_ctl_offset, 0x1E00ULL, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, write_msr(8, perf_ctl_offset, 0x1E00ULL, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, write_msr(12, perf_ctl_offset, 0x1E00ULL, perf_ctl_mask));
    m_msrio_group->write_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 0, 3e9);
    }

    // all cpus on core 1
    {
    EXPECT_CALL(*m_msrio, write_msr(1, perf_ctl_offset, 0x1E00ULL, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, write_msr(5, perf_ctl_offset, 0x1E00ULL, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, write_msr(9, perf_ctl_offset, 0x1E00ULL, perf_ctl_mask));
    EXPECT_CALL(*m_msrio, write_msr(13, perf_ctl_offset, 0x1E00ULL, perf_ctl_mask));
    m_msrio_group->write_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 1, 3e9);
    }

    // Set power limit to 300 W
    {
    uint64_t pl1_limit_offset = 0x610;
    uint64_t pl1_limit_mask = 0x7FFF;
    // all cpus on package 0
    EXPECT_CALL(*m_msrio, write_msr(0, pl1_limit_offset, 0x960ULL, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, write_msr(4, pl1_limit_offset, 0x960ULL, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, write_msr(8, pl1_limit_offset, 0x960ULL, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, write_msr(12, pl1_limit_offset, 0x960ULL, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, write_msr(1, pl1_limit_offset, 0x960ULL, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, write_msr(5, pl1_limit_offset, 0x960ULL, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, write_msr(9, pl1_limit_offset, 0x960ULL, pl1_limit_mask));
    EXPECT_CALL(*m_msrio, write_msr(13, pl1_limit_offset, 0x960ULL, pl1_limit_mask));
    m_msrio_group->write_control("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT", GEOPM_DOMAIN_PACKAGE, 0, 300);
    }

    // Set uncore frequency to 1.5GHz
    {
    uint64_t uncore_ratio_offset = 0x620;
    uint64_t uncore_min_mask = 0x7F00;
    uint64_t uncore_max_mask = 0x7F;
    // all cpus on package 0
    EXPECT_CALL(*m_msrio, write_msr(0, uncore_ratio_offset, 0xF00ULL, uncore_min_mask));
    EXPECT_CALL(*m_msrio, write_msr(4, uncore_ratio_offset, 0xF00ULL, uncore_min_mask));
    EXPECT_CALL(*m_msrio, write_msr(8, uncore_ratio_offset, 0xF00ULL, uncore_min_mask));
    EXPECT_CALL(*m_msrio, write_msr(12, uncore_ratio_offset, 0xF00ULL, uncore_min_mask));
    EXPECT_CALL(*m_msrio, write_msr(1, uncore_ratio_offset, 0xF00ULL, uncore_min_mask));
    EXPECT_CALL(*m_msrio, write_msr(5, uncore_ratio_offset, 0xF00ULL, uncore_min_mask));
    EXPECT_CALL(*m_msrio, write_msr(9, uncore_ratio_offset, 0xF00ULL, uncore_min_mask));
    EXPECT_CALL(*m_msrio, write_msr(13, uncore_ratio_offset, 0xF00ULL, uncore_min_mask));
    m_msrio_group->write_control("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", GEOPM_DOMAIN_PACKAGE, 0, 1.5e9);
    EXPECT_CALL(*m_msrio, write_msr(0, uncore_ratio_offset, 0xFULL, uncore_max_mask));
    EXPECT_CALL(*m_msrio, write_msr(4, uncore_ratio_offset, 0xFULL, uncore_max_mask));
    EXPECT_CALL(*m_msrio, write_msr(8, uncore_ratio_offset, 0xFULL, uncore_max_mask));
    EXPECT_CALL(*m_msrio, write_msr(12, uncore_ratio_offset, 0xFULL, uncore_max_mask));
    EXPECT_CALL(*m_msrio, write_msr(1, uncore_ratio_offset, 0xFULL, uncore_max_mask));
    EXPECT_CALL(*m_msrio, write_msr(5, uncore_ratio_offset, 0xFULL, uncore_max_mask));
    EXPECT_CALL(*m_msrio, write_msr(9, uncore_ratio_offset, 0xFULL, uncore_max_mask));
    EXPECT_CALL(*m_msrio, write_msr(13, uncore_ratio_offset, 0xFULL, uncore_max_mask));
    m_msrio_group->write_control("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", GEOPM_DOMAIN_PACKAGE, 0, 1.5e9);
    }

}

TEST_F(MSRIOGroupTest, allowlist)
{
    char file_name[NAME_MAX] = __FILE__;
    std::ifstream file(std::string(dirname(file_name)) + "/legacy_allowlist.out");
    std::string line;
    uint64_t offset;
    uint64_t mask;
    std::string comment;
    std::map<uint64_t, uint64_t> legacy_map;
    std::map<uint64_t, uint64_t> curr_map;
    while (std::getline(file, line)) {
        if (line.compare(0, 1, "#") == 0) continue;
        std::string tmp;
        size_t sz;
        std::istringstream iss(line);
        iss >> tmp;
        offset = std::stoull(tmp, &sz, 16);
        iss >> tmp;
        mask = std::stoull(tmp, &sz, 16);
        iss >> comment;// #
        iss >> comment;// comment
        legacy_map[offset] = mask;
    }

    std::string allowlist = MSRIOGroup::msr_allowlist(MSRIOGroup::M_CPUID_SKX);
    std::istringstream iss(allowlist);
    std::getline(iss, line);// throw away title line
    while (std::getline(iss, line)) {
        std::string tmp;
        size_t sz;
        std::istringstream iss(line);
        iss >> tmp;
        offset = std::stoull(tmp, &sz, 16);
        iss >> tmp;
        mask = std::stoull(tmp, &sz, 16);
        iss >> comment;// #
        iss >> comment;// comment
        curr_map[offset] = mask;
    }

    EXPECT_NE(0ull, curr_map.size()) << "Expected at least one register in allowlist.";

    for (auto it = curr_map.begin(); it != curr_map.end(); ++it) {
        offset = it->first;
        mask = it->second;
        auto leg_it = legacy_map.find(offset);
        if (leg_it == legacy_map.end()) {
            //not found error
            if (!mask) {
                FAIL() << std::setfill('0') << std::hex << "new read offset 0x"
                       << std::setw(8) << offset << " introduced";
            }
            continue;
        }
        uint64_t leg_mask = leg_it->second;
        EXPECT_EQ(mask, mask & leg_mask) << std::setfill('0') << std::hex
                                         << "offset 0x" << std::setw(8) << offset
                                         << "write mask change detected, from 0x"
                                         << std::setw(16) << leg_mask << " to 0x"
                                         << mask << " bitwise AND yields 0x"
                                         << (mask & leg_mask);
    }
}

TEST_F(MSRIOGroupTest, cpuid)
{
    FILE *pid = NULL;
    std::string command = "lscpu | grep 'Model name:' | grep 'Intel'";
    EXPECT_EQ(0, geopm_sched_popen(command.c_str(), &pid));
    if (pclose(pid) == 0) {
        command = "printf '%.2x%x\n' $(lscpu | grep 'CPU family:' | awk -F: '{print $2}') $(lscpu | grep 'Model:' | awk -F: '{print $2}')";
        int expected_cpuid = 0;
        pid = NULL;
        EXPECT_EQ(0, geopm_sched_popen(command.c_str(), &pid));
        EXPECT_EQ(1, fscanf(pid, "%x", &expected_cpuid));
        EXPECT_EQ(0, pclose(pid));
        int cpuid = m_msrio_group->cpuid();
        EXPECT_EQ(expected_cpuid, cpuid);
   }
   else {
       std::cerr << "Warning: skipping MSRIOGroupTest.cpuid because non-intel architecture detected" << std::endl;
   }
}

TEST_F(MSRIOGroupTest, parse_json_msrs_error_top_level)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs("{}}"),
                               GEOPM_ERROR_INVALID,
                               "detected a malformed json string");

    const std::map<std::string, Json> complete {
        {"msrs", {}}
    };
    std::map<std::string, Json> input = complete;

    // unexpected keys
    input["extra"] = "extra";
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "unexpected key \"extra\" found at top level");


    // expected keys
    std::vector<std::string> top_level = {"msrs"};
    for (auto key : top_level) {
        input = complete;
        input.erase(key);
        GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                                   GEOPM_ERROR_INVALID,
                                   "\"" + key + "\" key is required");
    }

    // check types
    input = complete;
    input["msrs"] = "none";
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"msrs\" must be an object at top level");

    input = complete;
    input["msrs"] = Json::object{ {"MSR_ONE", 1} };
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "msr \"MSR_ONE\" must be an object");
}

TEST_F(MSRIOGroupTest, parse_json_msrs_error_msrs)
{
    const std::map<std::string, Json> complete {
        {"offset", "0x10"},
        {"domain", "cpu"},
        {"fields", Json::object{}}
    };
    std::map<std::string, Json> input;
    std::map<std::string, Json> msr = complete;
    msr["extra"] = "extra";
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "unexpected key \"extra\" found in msr \"MSR_ONE\"");

    // required keys
    std::vector<std::string> msr_keys {"offset", "domain", "fields"};
    for (auto key : msr_keys) {
        msr = complete;
        msr.erase(key);
        input["msrs"] = Json::object{ {"MSR_ONE", msr} };
        GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                                   GEOPM_ERROR_INVALID,
                                   "\"" + key + "\" key is required in msr \"MSR_ONE\"");
    }

    // check types
    msr = complete;
    msr["offset"] = 10;
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"offset\" must be a hex string and non-zero in msr \"MSR_ONE\"");
    msr = complete;
    msr["offset"] = "invalid";
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"offset\" must be a hex string and non-zero in msr \"MSR_ONE\"");
    msr = complete;
    msr["domain"] = 3;
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"domain\" must be a valid domain string in msr \"MSR_ONE\"");
    msr = complete;
    msr["domain"] = "unknown";
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"domain\" must be a valid domain string in msr \"MSR_ONE\"");
    msr = complete;
    msr["fields"] = "none";
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"fields\" must be an object in msr \"MSR_ONE\"");
    msr = complete;
    msr["fields"] = Json::object{ {"FIELD_RO", 2} };
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"FIELD_RO\" field within msr \"MSR_ONE\" must be an object");
}

TEST_F(MSRIOGroupTest, parse_json_msrs_error_fields)
{
    const std::map<std::string, Json> header {
        {"offset", "0x10"},
        {"domain", "cpu"},
    };
    const std::map<std::string, Json> complete {
        {"begin_bit", 1},
        {"end_bit", 4},
        {"function", "scale"},
        {"units", "hertz"},
        {"scalar", 2},
        {"writeable", false},
        {"behavior", "variable"}
    };
    std::map<std::string, Json> fields, msr, input;
    // used to rebuild the Json object with the "fields" section updated
    auto reset_input = [header, complete, &fields, &msr, &input]() {
        msr = header;
        msr["fields"] = Json::object{ {"FIELD_RO", fields} };
        input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    };

    fields = complete;
    fields["extra"] = "extra";
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "unexpected key \"extra\" found in \"MSR_ONE:FIELD_RO\"");

    // required keys
    std::vector<std::string> field_keys {"begin_bit", "end_bit", "function",
                                         "units", "scalar", "writeable", "behavior"
    };
    for (auto key : field_keys) {
        fields = complete;
        fields.erase(key);
        reset_input();
        GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                                   GEOPM_ERROR_INVALID,
                                   "\"" + key + "\" key is required in \"MSR_ONE:FIELD_RO\"");
    }

    // check types
    fields = complete;
    fields["begin_bit"] = "one";
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"begin_bit\" must be an integer in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["begin_bit"] = 1.1;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"begin_bit\" must be an integer in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["end_bit"] = "four";
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"end_bit\" must be an integer in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["end_bit"] = 4.4;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"end_bit\" must be an integer in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["function"] = 2;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"function\" must be a valid function string in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["units"] = 3;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"units\" must be a string in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["scalar"] = "two";
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"scalar\" must be a number in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["writeable"] = 0;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"writeable\" must be a bool in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["aggregation"] = "invalid";
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"aggregation\" must be a valid aggregation function name in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["description"] = 1.0;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"description\" must be a string in \"MSR_ONE:FIELD_RO\"");

    fields = complete;
    fields["behavior"] = 1.0;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"behavior\" must be a valid behavior string in \"MSR_ONE:FIELD_RO\"");
}

TEST_F(MSRIOGroupTest, parse_json_msrs)
{
    std::string json = R"({ "msrs": {
           "MSR_ONE": { "offset": "0x12", "domain": "package",
               "fields": {
                   "FIELD_RO" : {
                       "begin_bit": 1,
                       "end_bit": 4,
                       "function": "scale",
                       "units": "hertz",
                       "scalar": 2,
                       "behavior": "variable",
                       "writeable": false,
                       "aggregation": "average",
                       "description": "a beautiful and clear description of a field"
                   }
               }
           },
           "MSR_TWO": { "offset": "0x10", "domain": "cpu",
               "fields": {
                   "FIELD_RW" : {
                       "begin_bit": 1,
                       "end_bit": 4,
                       "function": "scale",
                       "units": "hertz",
                       "scalar": 2,
                       "behavior": "label",
                       "writeable": true
                   }
               }
           }
    } } )";
    m_msrio_group->parse_json_msrs(json);
    auto signals = m_msrio_group->signal_names();
    std::set<std::string> expected_signals = {"MSR::MSR_ONE:FIELD_RO", "MSR::MSR_TWO:FIELD_RW"};
    for (const auto &name : expected_signals) {
        EXPECT_TRUE(signals.find(name) != signals.end()) << "Expected signal " << name << " not found in IOGroup.";
    }
    auto controls = m_msrio_group->control_names();
    std::set<std::string> expected_controls = {"MSR::MSR_TWO:FIELD_RW"};
    for (const auto &name : expected_controls) {
        EXPECT_TRUE(controls.find(name) != controls.end()) << "Expected control " << name << " not found in IOGroup.";
    }
    EXPECT_TRUE(is_agg_average(m_msrio_group->agg_function("MSR::MSR_ONE:FIELD_RO")));
    EXPECT_EQ("    description: a beautiful and clear description of a field\n"
              "    units: hertz\n"
              "    aggregation: average\n"
              "    domain: package\n"
              "    iogroup: MSRIOGroup",
              m_msrio_group->signal_description("MSR::MSR_ONE:FIELD_RO"));
}

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
        int m_num_package = 2;
        int m_num_core = 4;
        int m_num_cpu = 16;
        void mock_enable_fixed_counters(void);
};

class MSRIOGroupTestMockMSRIO : public geopm::MSRIOImp
{
    public:
        MSRIOGroupTestMockMSRIO(int num_cpu);
        virtual ~MSRIOGroupTestMockMSRIO();
        std::vector<std::string> test_dev_paths();
    protected:
        void msr_path(int cpu_idx,
                      int is_fallback,
                      std::string &path) override;
        void msr_batch_path(std::string &path) override;

        const size_t M_MAX_OFFSET;
        const int m_num_cpu;
        std::vector<std::string> m_test_dev_path;
};

MSRIOGroupTestMockMSRIO::MSRIOGroupTestMockMSRIO(int num_cpu)
    : MSRIOImp(num_cpu)
    , M_MAX_OFFSET(4096)
    , m_num_cpu(num_cpu)
{
    union field_u {
        uint64_t field;
        uint16_t off[4];
    };
    union field_u fu;
    for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
        char tmp_path[NAME_MAX] = "/tmp/test_platform_io_dev_cpu_XXXXXX";
        int fd = mkstemp(tmp_path);
        if (fd == -1) {
           throw Exception("MSRIOGroupTestMockMSRIO: mkstemp() failed",
                           errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_test_dev_path.push_back(tmp_path);

        int err = ftruncate(fd, M_MAX_OFFSET);
        if (err) {
            throw Exception("MSRIOGroupTestMockMSRIO: ftruncate() failed",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        uint64_t *contents = (uint64_t *)mmap(NULL, M_MAX_OFFSET, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (contents == NULL) {
            throw Exception("MSRIOGroupTestMockMSRIO: mmap() failed",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        close(fd);
        size_t num_field = M_MAX_OFFSET / sizeof(uint64_t);
        for (size_t field_idx = 0; field_idx < num_field; ++field_idx) {
            uint16_t offset = field_idx * sizeof(uint64_t);
            for (int off_idx = 0; off_idx < 4; ++off_idx) {
               fu.off[off_idx] = offset;
            }
            contents[field_idx] = fu.field;
        }
        munmap(contents, M_MAX_OFFSET);
    }
}

MSRIOGroupTestMockMSRIO::~MSRIOGroupTestMockMSRIO()
{
    for (auto &path : m_test_dev_path) {
        unlink(path.c_str());
    }
}

std::vector<std::string> MSRIOGroupTestMockMSRIO::test_dev_paths()
{
    return m_test_dev_path;
}

void MSRIOGroupTestMockMSRIO::msr_path(int cpu_idx,
                                       int is_fallback,
                                       std::string &path)
{
    path = m_test_dev_path[cpu_idx];
}

void MSRIOGroupTestMockMSRIO::msr_batch_path(std::string &path)
{
    path = "test_dev_msr_safe";
}

void MSRIOGroupTest::SetUp()
{
    m_topo = make_topo(m_num_package, m_num_core, m_num_cpu);
    // suppress warnings about num_domain and domain_nested calls
    EXPECT_CALL(*m_topo, num_domain(_)).Times(AtLeast(0));
    EXPECT_CALL(*m_topo, domain_nested(_, _, _)).Times(AtLeast(0));

    auto msrio = geopm::make_unique<MSRIOGroupTestMockMSRIO>(m_num_cpu);
    m_test_dev_path = msrio->test_dev_paths();
    m_msrio_group = geopm::make_unique<MSRIOGroup>(*m_topo, std::move(msrio),
                                                   MSRIOGroup::M_CPUID_SKX,
                                                   m_num_cpu);

    int fd = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd);
    uint64_t value;
    size_t num_read = pread(fd, &value, sizeof(value), 0x0);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x0ULL, value);
    num_read = pread(fd, &value, sizeof(value), 0x198);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x0198019801980198ULL, value);
    num_read = pread(fd, &value, sizeof(value), 0x1A0);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x01A001A001A001A0ULL, value);
    num_read = pread(fd, &value, sizeof(value), 0x610);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x0610061006100610ULL, value);
    close(fd);
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
    };
    for (auto id : cpuids) {
        auto msrio = geopm::make_unique<MSRIOGroupTestMockMSRIO>(m_num_cpu);
        try {
            MSRIOGroup(*m_topo, std::move(msrio), id, m_num_cpu);
        }
        catch (const std::exception &ex) {
            FAIL() << "Could not construct MSRIOGroup for cpuid 0x"
                   << std::hex << id << std::dec << ": " << ex.what();
        }
    }

    // unsupported cpuid
    auto msrio = geopm::make_unique<MSRIOGroupTestMockMSRIO>(m_num_cpu);
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup(*m_topo, std::move(msrio), 0x9999, m_num_cpu),
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
        // check signal aliases are valid
        EXPECT_TRUE(m_msrio_group->is_valid_signal(name));
        // check names appear in signal_names
        EXPECT_TRUE(signal_names.find(name) != signal_names.end());
        // check that there is some non-empty description
        EXPECT_FALSE(m_msrio_group->signal_description(name).empty());
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

    // push valid signals
    int freq_idx_0 = m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 0);
    ASSERT_EQ(0, freq_idx_0);
    int inst_idx_0 = m_msrio_group->push_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY",
                                                GEOPM_DOMAIN_CPU, 0);
    ASSERT_EQ(1, inst_idx_0);

    // pushing same signal gives same index
    int idx2 = m_msrio_group->push_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(inst_idx_0, idx2);

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
    int freq_idx_0 = m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 0);
    ASSERT_EQ(0, freq_idx_0);
    int inst_idx_0 = m_msrio_group->push_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY",
                                                GEOPM_DOMAIN_CPU, 0);
    int inst_idx_1 = m_msrio_group->push_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY",
                                                GEOPM_DOMAIN_CPU, 1);
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->sample(freq_idx_0),
                               GEOPM_ERROR_RUNTIME, "sample() called before signal was read");

    // write frequency values to be read
    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    int fd_1 = open(m_test_dev_path[1].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);
    ASSERT_NE(-1, fd_1);
    uint64_t value = 0xB00;
    size_t num_write = pwrite(fd_0, &value, sizeof(value), 0x198);
    ASSERT_EQ(num_write, sizeof(value));
    // write inst_retired value to be read
    value = 1234;
    num_write = pwrite(fd_0, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));
    value = 5678;
    num_write = pwrite(fd_1, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));

    m_msrio_group->read_batch();
    double freq_0 = m_msrio_group->sample(freq_idx_0);
    double inst_0 = m_msrio_group->sample(inst_idx_0);
    double inst_1 = m_msrio_group->sample(inst_idx_1);
    EXPECT_EQ(1.1e9, freq_0);
    EXPECT_EQ(1234, inst_0);
    EXPECT_EQ(5678, inst_1);

    // sample again without read should get same value
    freq_0 = m_msrio_group->sample(freq_idx_0);
    inst_0 = m_msrio_group->sample(inst_idx_0);
    inst_1 = m_msrio_group->sample(inst_idx_1);
    EXPECT_EQ(1.1e9, freq_0);
    EXPECT_EQ(1234, inst_0);
    EXPECT_EQ(5678, inst_1);

    // read_batch sees updated values
    value = 0xC00;
    num_write = pwrite(fd_0, &value, sizeof(value), 0x198);
    ASSERT_EQ(num_write, sizeof(value));
    value = 87654;
    num_write = pwrite(fd_0, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));
    value = 65432;
    num_write = pwrite(fd_1, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));
    m_msrio_group->read_batch();
    freq_0 = m_msrio_group->sample(freq_idx_0);
    inst_0 = m_msrio_group->sample(inst_idx_0);
    inst_1 = m_msrio_group->sample(inst_idx_1);
    EXPECT_EQ(1.2e9, freq_0);
    // note that 64-bit counters are normalized to first sample
    EXPECT_EQ(87654, inst_0);
    EXPECT_EQ(65432, inst_1);

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "cannot push a signal after read_batch");

    close(fd_0);
    close(fd_1);
}

TEST_F(MSRIOGroupTest, sample_raw)
{
    int inst_idx_0 = m_msrio_group->push_signal("MSR::FIXED_CTR0#",
                                                GEOPM_DOMAIN_CPU, 0);
    int inst_idx_1 = m_msrio_group->push_signal("MSR::FIXED_CTR0#",
                                                GEOPM_DOMAIN_CPU, 1);
    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    int fd_1 = open(m_test_dev_path[1].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);
    ASSERT_NE(-1, fd_1);
    uint64_t value = 0xB000D000F0001234;
    size_t num_write = pwrite(fd_0, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));
    value = 0xB000D000F0001235;
    num_write = pwrite(fd_1, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));

    m_msrio_group->read_batch();
    uint64_t inst_0 = geopm_signal_to_field(m_msrio_group->sample(inst_idx_0));
    uint64_t inst_1 = geopm_signal_to_field(m_msrio_group->sample(inst_idx_1));
    EXPECT_EQ(0xB000D000F0001234, inst_0);
    EXPECT_EQ(0xB000D000F0001235, inst_1);

    close(fd_0);
    close(fd_1);
}

TEST_F(MSRIOGroupTest, read_signal_energy)
{
    uint64_t pkg_energy_offset = 0x611;
    uint64_t dram_energy_offset = 0x619;
    uint64_t value = 0;
    size_t num_write = 0;
    double result;

    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);

    value = 1638400;  // 61uJ units
    num_write = pwrite(fd_0, &value, sizeof(value), pkg_energy_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("ENERGY_PACKAGE", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_NEAR(100, result, 0.0001);

    value = 3276799;  // 15uJ units
    num_write = pwrite(fd_0, &value, sizeof(value), dram_energy_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("ENERGY_DRAM", GEOPM_DOMAIN_BOARD_MEMORY, 0);
    EXPECT_NEAR(50, result, 0.0001);

    close(fd_0);
}

TEST_F(MSRIOGroupTest, read_signal_counter)
{
    uint64_t tsc_offset = 0x10;
    uint64_t fixed0_offset = 0x309;
    uint64_t fixed1_offset = 0x30A;
    uint64_t fixed2_offset = 0x30B;
    uint64_t value = 0;
    size_t num_write = 0;
    double result;

    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);

    value = 11111;
    num_write = pwrite(fd_0, &value, sizeof(value), tsc_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("MSR::TIME_STAMP_COUNTER:TIMESTAMP_COUNT", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(11111, result);
    value = 22222;
    num_write = pwrite(fd_0, &value, sizeof(value), tsc_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("TIMESTAMP_COUNTER", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(22222, result);

    value = 7777;
    num_write = pwrite(fd_0, &value, sizeof(value), fixed0_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(7777, result);
    value = 8888;
    num_write = pwrite(fd_0, &value, sizeof(value), fixed0_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("INSTRUCTIONS_RETIRED", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(8888, result);

    value = 33333;
    num_write = pwrite(fd_0, &value, sizeof(value), fixed1_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("MSR::FIXED_CTR1:CPU_CLK_UNHALTED_THREAD", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(33333, result);
    value = 44444;
    num_write = pwrite(fd_0, &value, sizeof(value), fixed1_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("CYCLES_THREAD", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(44444, result);

    value = 55555;
    num_write = pwrite(fd_0, &value, sizeof(value), fixed2_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("MSR::FIXED_CTR2:CPU_CLK_UNHALTED_REF_TSC", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(55555, result);
    value = 66666;
    num_write = pwrite(fd_0, &value, sizeof(value), fixed2_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("CYCLES_REFERENCE", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(66666, result);

    close(fd_0);
}

TEST_F(MSRIOGroupTest, read_signal_frequency)
{
    uint64_t status_offset = 0x198;
    uint64_t limit_offset = 0x1ad;
    uint64_t value = 0;
    size_t num_write = 0;
    double result;

    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);

    value = 0xD00;  // 100MHz units, field 15:8
    num_write = pwrite(fd_0, &value, sizeof(value), status_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(1.3e9, result);

    value = 0xE00;
    num_write = pwrite(fd_0, &value, sizeof(value), status_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("FREQUENCY", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(1.4e9, result);

    // For SKX: MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0 0:7
    value = 0xF;
    num_write = pwrite(fd_0, &value, sizeof(value), limit_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("FREQUENCY_MAX", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(1.5e9, result);

    close(fd_0);
}

TEST_F(MSRIOGroupTest, read_signal_temperature)
{
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::TEMPERATURE_TARGET:PROCHOT_MIN"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::THERM_STATUS:DIGITAL_READOUT"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::PACKAGE_THERM_STATUS:DIGITAL_READOUT"));

    // temperature is (PROCHOT_MIN - DIGITAL_READOUT)
    int prochot_val = 98;
    int readout_val = 66;
    double exp_temp = prochot_val - readout_val;

    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);

    uint64_t prochot_msr = 0x1A2;
    int prochot_begin = 16;
    uint64_t value = prochot_val << prochot_begin;
    size_t num_write = pwrite(fd_0, &value, sizeof(value), prochot_msr);
    ASSERT_EQ(num_write, sizeof(value));
    uint64_t readout_msr = 0x19C;
    int readout_begin = 16;
    value = readout_val << readout_begin;
    num_write = pwrite(fd_0, &value, sizeof(value), readout_msr);
    ASSERT_EQ(num_write, sizeof(value));
    EXPECT_NEAR(exp_temp, m_msrio_group->read_signal("TEMPERATURE_CORE", GEOPM_DOMAIN_CORE, 0), 0.001);

    readout_val = 55;
    exp_temp = prochot_val - readout_val;
    uint64_t pkg_readout_msr = 0x1B1;
    int pkg_readout_begin = 16;
    value = readout_val << pkg_readout_begin;
    num_write = pwrite(fd_0, &value, sizeof(value), pkg_readout_msr);
    ASSERT_EQ(num_write, sizeof(value));
    EXPECT_NEAR(exp_temp, m_msrio_group->read_signal("TEMPERATURE_PACKAGE", GEOPM_DOMAIN_PACKAGE, 0), 0.001);
}

TEST_F(MSRIOGroupTest, read_signal_power)
{
    uint64_t info_offset = 0x614;
    uint64_t value = 0;
    size_t num_write = 0;
    double result;

    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);

    // power limits
    value = 0x258;  // 1/8W units, 14:0
    num_write = pwrite(fd_0, &value, sizeof(value), info_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("MSR::PKG_POWER_INFO:THERMAL_SPEC_POWER", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(75, result);
    value = 0x262;
    num_write = pwrite(fd_0, &value, sizeof(value), info_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("POWER_PACKAGE_TDP", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(76.25, result);

    value = 0x1920000;  // 1/8W units, 30:16
    num_write = pwrite(fd_0, &value, sizeof(value), info_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("MSR::PKG_POWER_INFO:MIN_POWER", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(50.25, result);
    value = 0x3210000;
    num_write = pwrite(fd_0, &value, sizeof(value), info_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("POWER_PACKAGE_MIN", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(100.125, result);

    value = 0x64400000000;  // 1/8W units, 46:32
    num_write = pwrite(fd_0, &value, sizeof(value), info_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("MSR::PKG_POWER_INFO:MAX_POWER", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(200.5, result);
    value = 0x64B00000000;
    num_write = pwrite(fd_0, &value, sizeof(value), info_offset);
    ASSERT_EQ(num_write, sizeof(value));
    result = m_msrio_group->read_signal("POWER_PACKAGE_MAX", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(201.375, result);
}

TEST_F(MSRIOGroupTest, push_signal_temperature)
{
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::TEMPERATURE_TARGET:PROCHOT_MIN"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::THERM_STATUS:DIGITAL_READOUT"));
    ASSERT_TRUE(m_msrio_group->is_valid_signal("MSR::PACKAGE_THERM_STATUS:DIGITAL_READOUT"));

    int core_idx = m_msrio_group->push_signal("TEMPERATURE_CORE", GEOPM_DOMAIN_CORE, 0);
    int pkg_idx = m_msrio_group->push_signal("TEMPERATURE_PACKAGE", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_GE(core_idx, 0);
    EXPECT_GE(pkg_idx, 0);
    // temperature is (PROCHOT_MIN - DIGITAL_READOUT)
    int prochot_val = 98;
    int readout_val = 66;
    double exp_temp = prochot_val - readout_val;

    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);

    uint64_t prochot_msr = 0x1A2;
    int prochot_begin = 16;
    uint64_t value = prochot_val << prochot_begin;
    size_t num_write = pwrite(fd_0, &value, sizeof(value), prochot_msr);
    ASSERT_EQ(num_write, sizeof(value));
    uint64_t readout_msr = 0x19C;
    int readout_begin = 16;
    value = readout_val << readout_begin;
    num_write = pwrite(fd_0, &value, sizeof(value), readout_msr);
    ASSERT_EQ(num_write, sizeof(value));

    m_msrio_group->read_batch();
    EXPECT_NEAR(exp_temp, m_msrio_group->sample(core_idx), 0.001);

    readout_val = 55;
    exp_temp = prochot_val - readout_val;
    uint64_t pkg_readout_msr = 0x1B1;
    int pkg_readout_begin = 16;
    value = readout_val << pkg_readout_begin;
    num_write = pwrite(fd_0, &value, sizeof(value), pkg_readout_msr);
    ASSERT_EQ(num_write, sizeof(value));

    m_msrio_group->read_batch();
    EXPECT_NEAR(exp_temp, m_msrio_group->sample(pkg_idx), 0.001);
}

TEST_F(MSRIOGroupTest, signal_alias)
{
    int freq_idx = m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_CPU, 0);
    ASSERT_EQ(0, freq_idx);
    int alias = m_msrio_group->push_signal("FREQUENCY", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(freq_idx, alias);

    int fd = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd);
    uint64_t value = 0xB00;
    size_t num_write = pwrite(fd, &value, sizeof(value), 0x198);
    ASSERT_EQ(num_write, sizeof(value));

    m_msrio_group->read_batch();
    double freq = m_msrio_group->sample(alias);
    EXPECT_EQ(1.1e9, freq);

    close(fd);
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
    int freq_idx_0 = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 0);
    ASSERT_EQ(0, freq_idx_0);
    int power_idx = m_msrio_group->push_control("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT", GEOPM_DOMAIN_PACKAGE, 0);
    ASSERT_EQ(1, power_idx);

    // pushing same control gives same index
    int idx2 = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 0);
    EXPECT_EQ(freq_idx_0, idx2);

    // all provided controls are valid
    EXPECT_NE(0u, m_msrio_group->control_names().size());
    for (const auto &sig : m_msrio_group->control_names()) {
        EXPECT_TRUE(m_msrio_group->is_valid_control(sig));
    }
}

TEST_F(MSRIOGroupTest, adjust)
{
    int freq_idx_0 = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 0);
    int power_idx = m_msrio_group->push_control("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT", GEOPM_DOMAIN_PACKAGE, 0);

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_batch(), GEOPM_ERROR_INVALID,
                               "called before all controls were adjusted");

    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);
    uint64_t value;
    size_t num_read;
    // Set frequency to 1 GHz, power to 100W
    m_msrio_group->adjust(freq_idx_0, 1e9);
    m_msrio_group->adjust(power_idx, 160);
    m_msrio_group->write_batch();
    num_read = pread(fd_0, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0xA00ULL, (value & 0xFF00));
    num_read = pread(fd_0, &value, sizeof(value), 0x610);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x500ULL, (value & 0x3FFF));

    // Set frequency to 5 GHz, power to 200W
    m_msrio_group->adjust(freq_idx_0, 5e9);
    m_msrio_group->adjust(power_idx, 200);
    // Calling adjust without calling write_batch() should not
    // change the platform.
    num_read = pread(fd_0, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0xA00ULL, (value & 0xFF00));
    num_read = pread(fd_0, &value, sizeof(value), 0x610);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x500ULL, (value & 0x7FFF));

    m_msrio_group->write_batch();
    // Now that write_batch() been called the value on the platform
    // should be updated.
    num_read = pread(fd_0, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x3200ULL, (value & 0xFF00));
    num_read = pread(fd_0, &value, sizeof(value), 0x610);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x640ULL, (value & 0x7FFF));

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "cannot push a control after read_batch() or adjust()");

    close(fd_0);
}

TEST_F(MSRIOGroupTest, write_control)
{
    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);
    uint64_t value;
    size_t num_read;

    // Set frequency to 3 GHz immediately
    m_msrio_group->write_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 0, 3e9);
    num_read = pread(fd_0, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x1E00ULL, (value & 0xFF00));

    m_msrio_group->write_control("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT", GEOPM_DOMAIN_PACKAGE, 0, 300);
    num_read = pread(fd_0, &value, sizeof(value), 0x610);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x960ULL, (value & 0x7FFF));

    close(fd_0);
}

TEST_F(MSRIOGroupTest, control_alias)
{
    int freq_idx = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_CORE, 0);
    ASSERT_EQ(0, freq_idx);
    int alias = m_msrio_group->push_control("FREQUENCY", GEOPM_DOMAIN_CORE, 0);
    ASSERT_EQ(freq_idx, alias);
    int fd = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd);
    uint64_t value;
    size_t num_read;
    // Set frequency to 1 GHz
    m_msrio_group->adjust(freq_idx, 2e9);
    m_msrio_group->adjust(alias, 1e9); // will overwrite
    m_msrio_group->write_batch();
    num_read = pread(fd, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0xA00ULL, (value & 0xFF00));
    close(fd);
}

TEST_F(MSRIOGroupTest, whitelist)
{
    std::ifstream file("test/legacy_whitelist.out");
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

    std::string whitelist = m_msrio_group->msr_whitelist();
    std::istringstream iss(whitelist);
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

TEST_F(MSRIOGroupTest, register_msr_control)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_control("TEST"),
                               GEOPM_ERROR_INVALID, "msr_name_field must be of the form \"MSR::<msr_name>:<field_name>\"");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_control("MSR::TEST"),
                               GEOPM_ERROR_INVALID, "msr_name_field must be of the form \"MSR::<msr_name>:<field_name>\"");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_control("MSR::PERF_CTL:FREQ"),
                               GEOPM_ERROR_INVALID, "control_name MSR::PERF_CTL:FREQ was previously registered");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_control("MSR::BAD:BAD"),
                               GEOPM_ERROR_INVALID, "msr_name could not be found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_control("MSR::PERF_CTL:BAD"),
                               GEOPM_ERROR_INVALID, "field_name: BAD could not be found");

}

TEST_F(MSRIOGroupTest, parse_json_msrs_error_top_level)
{
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs("{}}"),
                               GEOPM_ERROR_INVALID,
                               "detected a malformed json string");

    const std::map<std::string, Json> complete {
        {"msrs", {}}
    };
    std::map<std::string, Json> input = complete;

    // unexpected keys
    input["extra"] = "extra";
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "unexpected key \"extra\" found at top level");


    // expected keys
    std::vector<std::string> top_level = {"msrs"};
    for (auto key : top_level) {
        input = complete;
        input.erase(key);
        GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                                   GEOPM_ERROR_INVALID,
                                   "\"" + key + "\" key is required");
    }

    // check types
    input = complete;
    input["msrs"] = "none";
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"msrs\" must be an object at top level");

    input = complete;
    input["msrs"] = Json::object{ {"MSR_ONE", 1} };
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
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
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "unexpected key \"extra\" found in msr \"MSR_ONE\"");

    // required keys
    std::vector<std::string> msr_keys {"offset", "domain", "fields"};
    for (auto key : msr_keys) {
        msr = complete;
        msr.erase(key);
        input["msrs"] = Json::object{ {"MSR_ONE", msr} };
        GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                                   GEOPM_ERROR_INVALID,
                                   "\"" + key + "\" key is required in msr \"MSR_ONE\"");
    }

    // check types
    msr = complete;
    msr["offset"] = 10;
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"offset\" must be a hex string in msr \"MSR_ONE\"");
    msr = complete;
    msr["offset"] = "invalid";
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"offset\" must be a hex string in msr \"MSR_ONE\"");
    msr = complete;
    msr["domain"] = 3;
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"domain\" must be a valid domain string in msr \"MSR_ONE\"");
    msr = complete;
    msr["domain"] = "unknown";
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"domain\" must be a valid domain string in msr \"MSR_ONE\"");
    msr = complete;
    msr["fields"] = "none";
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"fields\" must be an object in msr \"MSR_ONE\"");
    msr = complete;
    msr["fields"] = Json::object{ {"FIELD_RO", 2} };
    input["msrs"] = Json::object{ {"MSR_ONE", msr} };
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
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
        {"writeable", false}
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
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "unexpected key \"extra\" found in \"MSR_ONE:FIELD_RO\"");

    // required keys
    std::vector<std::string> field_keys {"begin_bit", "end_bit", "function",
                                         "units", "scalar", "writeable"};
    for (auto key : field_keys) {
        fields = complete;
        fields.erase(key);
        reset_input();
        GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                                   GEOPM_ERROR_INVALID,
                                   "\"" + key + "\" key is required in \"MSR_ONE:FIELD_RO\"");
    }

    // check types
    fields = complete;
    fields["begin_bit"] = "one";
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"begin_bit\" must be an integer in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["begin_bit"] = 1.1;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"begin_bit\" must be an integer in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["end_bit"] = "four";
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"end_bit\" must be an integer in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["end_bit"] = 4.4;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"end_bit\" must be an integer in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["function"] = 2;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"function\" must be a valid function string in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["units"] = 3;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"units\" must be a string in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["scalar"] = "two";
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"scalar\" must be a number in \"MSR_ONE:FIELD_RO\"");
    fields = complete;
    fields["writeable"] = 0;
    reset_input();
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup::parse_json_msrs(Json(input).dump()),
                               GEOPM_ERROR_INVALID,
                               "\"writeable\" must be a bool in \"MSR_ONE:FIELD_RO\"");
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
                       "writeable": false
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
                       "writeable": true
                   }
               }
           }
    } } )";
    auto msr_list = MSRIOGroup::parse_json_msrs(json);

    ASSERT_EQ(1u, msr_list.size());

    auto &msr1 = msr_list[0];
    EXPECT_EQ("MSR_TWO", msr1->name());
    EXPECT_EQ(0x10U, msr1->offset());
    EXPECT_EQ(GEOPM_DOMAIN_CPU, msr1->domain_type());
    EXPECT_EQ(1, msr1->num_control());
    EXPECT_EQ("FIELD_RW", msr1->control_name(0));
}

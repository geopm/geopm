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
using testing::NiceMock;
using json11::Json;

bool is_format_double(std::function<std::string(double)> func);
bool is_format_integer(std::function<std::string(double)> func);
bool is_format_raw64(std::function<std::string(double)> func);

class MSRIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        std::vector<std::string> m_test_dev_path;
        std::unique_ptr<MSRIOGroup> m_msrio_group;
        NiceMock<MockPlatformTopo> m_topo;
        int m_num_cpu = 16;
        void mock_enable_fixed_counters(void);
};

class MockMSRIO : public geopm::MSRIOImp
{
    public:
        MockMSRIO(int num_cpu);
        virtual ~MockMSRIO();
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

MockMSRIO::MockMSRIO(int num_cpu)
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
           throw Exception("MockMSRIO: mkstemp() failed",
                           errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_test_dev_path.push_back(tmp_path);

        int err = ftruncate(fd, M_MAX_OFFSET);
        if (err) {
            throw Exception("MockMSRIO: ftruncate() failed",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        uint64_t *contents = (uint64_t *)mmap(NULL, M_MAX_OFFSET, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (contents == NULL) {
            throw Exception("MockMSRIO: mmap() failed",
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

MockMSRIO::~MockMSRIO()
{
    for (auto &path : m_test_dev_path) {
        unlink(path.c_str());
    }
}

std::vector<std::string> MockMSRIO::test_dev_paths()
{
    return m_test_dev_path;
}

void MockMSRIO::msr_path(int cpu_idx,
                         int is_fallback,
                         std::string &path)
{
    path = m_test_dev_path[cpu_idx];
}

void MockMSRIO::msr_batch_path(std::string &path)
{
    path = "test_dev_msr_safe";
}

void MSRIOGroupTest::SetUp()
{
    std::unique_ptr<MockMSRIO> msrio(new MockMSRIO(m_num_cpu));
    m_test_dev_path = msrio->test_dev_paths();
    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_PACKAGE)).WillByDefault(Return(1));
    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU)).WillByDefault(Return(m_num_cpu));
    std::set<int> package_cpus;
    for (int ii = 0; ii < m_num_cpu; ++ii) {
        ON_CALL(m_topo, domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_CPU, ii))
            .WillByDefault(Return(std::set<int>{ii}));
        package_cpus.insert(ii);
    }
    ON_CALL(m_topo, domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_PACKAGE, _))
        .WillByDefault(Return(package_cpus));

    m_msrio_group = std::unique_ptr<MSRIOGroup>(new MSRIOGroup(m_topo, std::move(msrio), 0x657, m_num_cpu)); // KNL cpuid

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
        std::unique_ptr<MockMSRIO> msrio(new MockMSRIO(m_num_cpu));
        try {
            MSRIOGroup(m_topo, std::move(msrio), id, m_num_cpu);
        }
        catch (const std::exception &ex) {
            FAIL() << "Could not construct MSRIOGroup for cpuid 0x"
                   << std::hex << id << std::dec << ": " << ex.what();
        }
    }

    // unsupported cpuid
    std::unique_ptr<MockMSRIO> msrio(new MockMSRIO(m_num_cpu));
    GEOPM_EXPECT_THROW_MESSAGE(MSRIOGroup(m_topo, std::move(msrio), 0x9999, m_num_cpu),
                               GEOPM_ERROR_RUNTIME, "Unsupported CPUID");
}

TEST_F(MSRIOGroupTest, signal_error)
{
    // error cases for push_signal
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_PACKAGE, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_PACKAGE, 9000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    // sample
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->sample(-1), GEOPM_ERROR_INVALID, "signal_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->sample(22), GEOPM_ERROR_INVALID, "signal_idx out of range");

    // read_signal
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->read_signal("INVALID", GEOPM_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->read_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_PACKAGE, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->read_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_PACKAGE, 9000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
}

TEST_F(MSRIOGroupTest, push_signal)
{
    EXPECT_TRUE(m_msrio_group->is_valid_signal("MSR::PERF_STATUS:FREQ"));
    EXPECT_FALSE(m_msrio_group->is_valid_signal("INVALID"));
    EXPECT_EQ(GEOPM_DOMAIN_CPU, m_msrio_group->signal_domain_type("MSR::FIXED_CTR0:INST_RETIRED_ANY"));
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_msrio_group->signal_domain_type("INVALID"));

    // push valid signals
    int freq_idx_0 = m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_PACKAGE, 0);
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
    int freq_idx_0 = m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_PACKAGE, 0);
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

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_PACKAGE, 0),
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

TEST_F(MSRIOGroupTest, read_signal)
{
    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);

    // read and sample immediately
    uint64_t value = 0xD00;
    size_t num_write = pwrite(fd_0, &value, sizeof(value), 0x198);
    ASSERT_EQ(num_write, sizeof(value));
    double freq_0 = m_msrio_group->read_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(1.3e9, freq_0);

    value = 7777;
    num_write = pwrite(fd_0, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));

    double inst_0 = m_msrio_group->read_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY", GEOPM_DOMAIN_CPU, 0);
    // 64-bit counters are normalized to first sample
    EXPECT_EQ(7777, inst_0);
    value = 8888;
    num_write = pwrite(fd_0, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));
    inst_0 = m_msrio_group->read_signal("MSR::FIXED_CTR0:INST_RETIRED_ANY", GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(8888, inst_0);

    close(fd_0);
}

TEST_F(MSRIOGroupTest, signal_alias)
{
    int freq_idx = m_msrio_group->push_signal("MSR::PERF_STATUS:FREQ", GEOPM_DOMAIN_PACKAGE, 0);
    ASSERT_EQ(0, freq_idx);
    int alias = m_msrio_group->push_signal("FREQUENCY", GEOPM_DOMAIN_PACKAGE, 0);
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
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_PACKAGE, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_PACKAGE, 9000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    // adjust
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->adjust(-1, 0), GEOPM_ERROR_INVALID, "control_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->adjust(22, 0), GEOPM_ERROR_INVALID, "control_idx out of range");

    // write_control
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_control("INVALID", GEOPM_DOMAIN_CPU, 0, 1e9),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_PACKAGE, -1, 1e9),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_PACKAGE, 9000, 1e9),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
}

TEST_F(MSRIOGroupTest, push_control)
{
    EXPECT_TRUE(m_msrio_group->is_valid_control("MSR::PERF_CTL:FREQ"));
    EXPECT_FALSE(m_msrio_group->is_valid_control("INVALID"));
    EXPECT_EQ(GEOPM_DOMAIN_CPU, m_msrio_group->control_domain_type("MSR::FIXED_CTR_CTRL:EN0_OS"));
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_msrio_group->control_domain_type("INVALID"));

    // push valid controls
    int freq_idx_0 = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_PACKAGE, 0);
    ASSERT_EQ(0, freq_idx_0);
    int power_idx = m_msrio_group->push_control("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT", GEOPM_DOMAIN_PACKAGE, 0);
    ASSERT_EQ(1, power_idx);

    // pushing same control gives same index
    int idx2 = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(freq_idx_0, idx2);

    // all provided controls are valid
    EXPECT_NE(0u, m_msrio_group->control_names().size());
    for (const auto &sig : m_msrio_group->control_names()) {
        EXPECT_TRUE(m_msrio_group->is_valid_control(sig));
    }
}

TEST_F(MSRIOGroupTest, adjust)
{
    int freq_idx_0 = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_PACKAGE, 0);
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
    m_msrio_group->write_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_PACKAGE, 0, 3e9);
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
    int freq_idx = m_msrio_group->push_control("MSR::PERF_CTL:FREQ", GEOPM_DOMAIN_PACKAGE, 0);
    ASSERT_EQ(0, freq_idx);
    int alias = m_msrio_group->push_control("FREQUENCY", GEOPM_DOMAIN_PACKAGE, 0);
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

TEST_F(MSRIOGroupTest, format_function)
{
    std::function<std::string(double)> func;
    func = m_msrio_group->format_function("MSR::PERF_STATUS:FREQ");
    EXPECT_TRUE(is_format_double(func));
    func = m_msrio_group->format_function("MSR::FIXED_CTR0:INST_RETIRED_ANY");
    EXPECT_TRUE(is_format_integer(func));
    func = m_msrio_group->format_function("MSR::PERF_STATUS#");
    EXPECT_TRUE(is_format_raw64(func));
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->format_function("INVALID"),
                               GEOPM_ERROR_INVALID, "not valid for MSRIOGroup");
}

TEST_F(MSRIOGroupTest, register_msr_signal)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_signal("TEST"),
                               GEOPM_ERROR_INVALID, "msr_name_field must be of the form \"MSR::<msr_name>:<field_name>\"");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_signal("MSR::TEST"),
                               GEOPM_ERROR_INVALID, "msr_name_field must be of the form \"MSR::<msr_name>:<field_name>\"");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_signal("MSR::PERF_STATUS:FREQ"),
                               GEOPM_ERROR_INVALID, "signal_name MSR::PERF_STATUS:FREQ was previously registered");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_signal("MSR::BAD:BAD"),
                               GEOPM_ERROR_INVALID, "msr_name could not be found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_signal("MSR::PERF_STATUS:BAD"),
                               GEOPM_ERROR_INVALID, "field_name: BAD could not be found");
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
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->register_msr_control("MSR::PERF_STATUS:BAD"),
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
    ASSERT_EQ(2u, msr_list.size());
    auto &msr0 = msr_list[0];
    EXPECT_EQ("MSR_ONE", msr0->name());
    EXPECT_EQ(0x12U, msr0->offset());
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE, msr0->domain_type());
    EXPECT_EQ(1, msr0->num_signal());
    EXPECT_EQ(0, msr0->num_control());
    EXPECT_EQ("FIELD_RO", msr0->signal_name(0));

    auto &msr1 = msr_list[1];
    EXPECT_EQ("MSR_TWO", msr1->name());
    EXPECT_EQ(0x10U, msr1->offset());
    EXPECT_EQ(GEOPM_DOMAIN_CPU, msr1->domain_type());
    EXPECT_EQ(1, msr1->num_signal());
    EXPECT_EQ(1, msr1->num_control());
    EXPECT_EQ("FIELD_RW", msr1->signal_name(0));
    EXPECT_EQ("FIELD_RW", msr1->control_name(0));
}

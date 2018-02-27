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

#include "geopm_sched.h"
#include "PlatformTopo.hpp"
#include "MSRIO.hpp"
#include "Exception.hpp"
#include "MSRIOGroup.hpp"
#include "geopm_test.hpp"

using geopm::MSRIOGroup;

class MSRIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        std::vector<std::string> m_test_dev_path;
        std::unique_ptr<geopm::MSRIOGroup> m_msrio_group;
};

class MockMSRIO : public geopm::MSRIO
{
    public:
        MockMSRIO();
        virtual ~MockMSRIO();
        std::vector<std::string> test_dev_paths();
    protected:
        void msr_path(int cpu_idx,
                      bool is_fallback,
                      std::string &path) override;
        void msr_batch_path(std::string &path) override;

        const size_t M_MAX_OFFSET;
        const int m_num_cpu;
        std::vector<std::string> m_test_dev_path;
};

MockMSRIO::MockMSRIO()
   : M_MAX_OFFSET(4096)
   , m_num_cpu(16)
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
           throw geopm::Exception("MockMSRIO: mkstemp() failed",
                                  errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_test_dev_path.push_back(tmp_path);

        int err = ftruncate(fd, M_MAX_OFFSET);
        if (err) {
            throw geopm::Exception("MockMSRIO: ftruncate() failed",
                                   errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        uint64_t *contents = (uint64_t *)mmap(NULL, M_MAX_OFFSET, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (contents == NULL) {
            throw geopm::Exception("MockMSRIO: mmap() failed",
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
                         bool is_fallback,
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
    std::unique_ptr<MockMSRIO> msrio(new MockMSRIO);
    m_test_dev_path = msrio->test_dev_paths();
    m_msrio_group = std::unique_ptr<MSRIOGroup>(new MSRIOGroup(std::move(msrio), 0x657, 16)); // KNL cpuid

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

TEST_F(MSRIOGroupTest, signal)
{
    // error cases
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("PERF_STATUS:FREQ", 99, 0),
                               GEOPM_ERROR_NOT_IMPLEMENTED, "non-CPU domain_type");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("INVALID", geopm::IPlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("PERF_STATUS:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of bounds");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("PERF_STATUS:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 9000),
                               GEOPM_ERROR_INVALID, "domain_idx out of bounds");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->sample(-1), GEOPM_ERROR_INVALID, "signal_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->sample(22), GEOPM_ERROR_INVALID, "signal_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->read_signal("PERF_STATUS:FREQ", 99, 0),
                               GEOPM_ERROR_NOT_IMPLEMENTED, "non-CPU domain_type");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->read_signal("INVALID", geopm::IPlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "signal name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->read_signal("PERF_STATUS:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of bounds");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->read_signal("PERF_STATUS:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 9000),
                               GEOPM_ERROR_INVALID, "domain_idx out of bounds");

    EXPECT_TRUE(m_msrio_group->is_valid_signal("PERF_STATUS:FREQ"));
    EXPECT_FALSE(m_msrio_group->is_valid_signal("INVALID"));

    // check domains
    EXPECT_EQ(geopm::PlatformTopo::M_DOMAIN_CPU, m_msrio_group->signal_domain_type("PERF_FIXED_CTR0:INST_RETIRED_ANY"));
    EXPECT_EQ(geopm::PlatformTopo::M_DOMAIN_INVALID, m_msrio_group->signal_domain_type("INVALID"));

    // push valid signals
    int freq_idx_0 = m_msrio_group->push_signal("PERF_STATUS:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 0);
    ASSERT_EQ(0, freq_idx_0);
    int inst_idx = m_msrio_group->push_signal("PERF_FIXED_CTR0:INST_RETIRED_ANY",
                                              geopm::IPlatformTopo::M_DOMAIN_CPU, 0);
    ASSERT_EQ(1, inst_idx);

    // pushing same signal gives same index
    int idx2 = m_msrio_group->push_signal("PERF_STATUS:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(freq_idx_0, idx2);

    // pushing same signal for another cpu gives different index
    int freq_idx_1 = m_msrio_group->push_signal("PERF_STATUS:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 1);
    EXPECT_NE(freq_idx_0, freq_idx_1);

    // TODO: same name different domain gives different index.

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->sample(freq_idx_0),
                         GEOPM_ERROR_RUNTIME, "sample.* called before signal was read");

    // write frequency values to be read
    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    int fd_1 = open(m_test_dev_path[1].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);
    ASSERT_NE(-1, fd_1);
    uint64_t value = 0xB00;
    size_t num_write = pwrite(fd_0, &value, sizeof(value), 0x198);
    ASSERT_EQ(num_write, sizeof(value));
    value = 0xC00;
    num_write = pwrite(fd_1, &value, sizeof(value), 0x198);
    ASSERT_EQ(num_write, sizeof(value));
    // write inst_retired value to be read
    value = 5678;
    num_write = pwrite(fd_0, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));

    m_msrio_group->read_batch();
    double freq_0 = m_msrio_group->sample(freq_idx_0);
    double freq_1 = m_msrio_group->sample(freq_idx_1);
    double inst = m_msrio_group->sample(inst_idx);
    EXPECT_EQ(1.1e9, freq_0);
    EXPECT_EQ(1.2e9, freq_1);
    EXPECT_EQ(5678, inst);

    // sample again without read should get same value
    freq_0 = m_msrio_group->sample(freq_idx_0);
    freq_1 = m_msrio_group->sample(freq_idx_1);
    inst = m_msrio_group->sample(inst_idx);
    EXPECT_EQ(1.1e9, freq_0);
    EXPECT_EQ(1.2e9, freq_1);
    EXPECT_EQ(5678, inst);

    // read_batch sees updated values
    value = 0xC00;
    num_write = pwrite(fd_0, &value, sizeof(value), 0x198);
    ASSERT_EQ(num_write, sizeof(value));
    value = 0xD00;
    num_write = pwrite(fd_1, &value, sizeof(value), 0x198);
    ASSERT_EQ(num_write, sizeof(value));

    value = 65432;
    num_write = pwrite(fd_0, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));
    m_msrio_group->read_batch();
    freq_0 = m_msrio_group->sample(freq_idx_0);
    freq_1 = m_msrio_group->sample(freq_idx_1);
    inst = m_msrio_group->sample(inst_idx);
    EXPECT_EQ(1.2e9, freq_0);
    EXPECT_EQ(1.3e9, freq_1);
    EXPECT_EQ(65432, inst);

    // read and sample immediately
    value = 0xD00;
    num_write = pwrite(fd_0, &value, sizeof(value), 0x198);
    ASSERT_EQ(num_write, sizeof(value));
    freq_0 = m_msrio_group->read_signal("PERF_STATUS:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(1.3e9, freq_0);
    value = 0xE00;
    num_write = pwrite(fd_1, &value, sizeof(value), 0x198);
    ASSERT_EQ(num_write, sizeof(value));
    freq_1 = m_msrio_group->read_signal("PERF_STATUS:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 1);
    EXPECT_EQ(1.4e9, freq_1);
    value = 7777;
    num_write = pwrite(fd_0, &value, sizeof(value), 0x309);
    ASSERT_EQ(num_write, sizeof(value));
    inst = m_msrio_group->read_signal("PERF_FIXED_CTR0:INST_RETIRED_ANY", geopm::IPlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(7777, inst);

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_signal("PERF_STATUS:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "cannot push a signal after read_batch");
}

TEST_F(MSRIOGroupTest, control)
{
    // error cases
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("PERF_CTL:FREQ", 99, 0),
                               GEOPM_ERROR_NOT_IMPLEMENTED, "non-CPU domain_type");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("INVALID", geopm::IPlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("PERF_CTL:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of bounds");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("PERF_CTL:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 9000),
                               GEOPM_ERROR_INVALID, "domain_idx out of bounds");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->adjust(-1, 0), GEOPM_ERROR_INVALID, "control_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->adjust(22, 0), GEOPM_ERROR_INVALID, "control_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_control("PERF_CTL:FREQ", 99, 0, 1e9),
                               GEOPM_ERROR_NOT_IMPLEMENTED, "non-CPU domain_type");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_control("INVALID", geopm::IPlatformTopo::M_DOMAIN_CPU, 0, 1e9),
                               GEOPM_ERROR_INVALID, "control name \"INVALID\" not found");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_control("PERF_CTL:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, -1, 1e9),
                               GEOPM_ERROR_INVALID, "domain_idx out of bounds");
    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_control("PERF_CTL:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 9000, 1e9),
                               GEOPM_ERROR_INVALID, "domain_idx out of bounds");
    EXPECT_TRUE(m_msrio_group->is_valid_control("PERF_CTL:FREQ"));
    EXPECT_FALSE(m_msrio_group->is_valid_control("INVALID"));

    EXPECT_EQ(geopm::PlatformTopo::M_DOMAIN_CPU, m_msrio_group->control_domain_type("PERF_FIXED_CTR_CTRL:EN0_OS"));
    EXPECT_EQ(geopm::PlatformTopo::M_DOMAIN_INVALID, m_msrio_group->control_domain_type("INVALID"));

    // push valid controls
    int freq_idx_0 = m_msrio_group->push_control("PERF_CTL:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 0);
    ASSERT_EQ(0, freq_idx_0);
    int power_idx = m_msrio_group->push_control("PKG_POWER_LIMIT:SOFT_POWER_LIMIT", geopm::IPlatformTopo::M_DOMAIN_CPU, 0);
    ASSERT_EQ(1, power_idx);

    // pushing same control gives same index
    int idx2 = m_msrio_group->push_control("PERF_CTL:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 0);
    EXPECT_EQ(freq_idx_0, idx2);

    // pushing same control for another cpu gives different index
    int freq_idx_1 = m_msrio_group->push_control("PERF_CTL:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 1);
    EXPECT_NE(freq_idx_0, freq_idx_1);

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->write_batch(), GEOPM_ERROR_INVALID,
                               "called before all controls were adjusted");

    int fd_0 = open(m_test_dev_path[0].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_0);
    int fd_1 = open(m_test_dev_path[1].c_str(), O_RDWR);
    ASSERT_NE(-1, fd_1);
    uint64_t value;
    size_t num_read;
    // Set frequency to 1 GHz, power to 100W
    m_msrio_group->adjust(freq_idx_0, 1e9);
    m_msrio_group->adjust(freq_idx_1, 1.1e9);
    m_msrio_group->adjust(power_idx, 160);
    m_msrio_group->write_batch();
    num_read = pread(fd_0, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0xA00ULL, (value & 0xFF00));
    num_read = pread(fd_1, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0xB00ULL, (value & 0xFF00));
    num_read = pread(fd_0, &value, sizeof(value), 0x610);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x500ULL, (value & 0x3FFF));

    // Set frequency to 5 GHz, power to 200W
    m_msrio_group->adjust(freq_idx_0, 5e9);
    m_msrio_group->adjust(freq_idx_1, 5.1e9);
    m_msrio_group->adjust(power_idx, 200);
    // Calling adjust without calling write_batch() should not
    // change the platform.
    num_read = pread(fd_0, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0xA00ULL, (value & 0xFF00));
    num_read = pread(fd_1, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0xB00ULL, (value & 0xFF00));
    num_read = pread(fd_0, &value, sizeof(value), 0x610);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x500ULL, (value & 0x7FFF));

    m_msrio_group->write_batch();
    // Now that write_batch() been called the value on the platform
    // should be updated.
    num_read = pread(fd_0, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x3200ULL, (value & 0xFF00));
    num_read = pread(fd_1, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x3300ULL, (value & 0xFF00));
    num_read = pread(fd_0, &value, sizeof(value), 0x610);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x640ULL, (value & 0x7FFF));

    // Set frequency to 3 GHz immediately
    m_msrio_group->write_control("PERF_CTL:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 0, 3e9);
    num_read = pread(fd_0, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x1E00ULL, (value & 0xFF00));
    m_msrio_group->write_control("PERF_CTL:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 1, 3.1e9);
    num_read = pread(fd_1, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x1F00ULL, (value & 0xFF00));

    m_msrio_group->write_control("PKG_POWER_LIMIT:SOFT_POWER_LIMIT", geopm::IPlatformTopo::M_DOMAIN_CPU, 0, 300);
    num_read = pread(fd_0, &value, sizeof(value), 0x610);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x960ULL, (value & 0x7FFF));

    close(fd_0);
    close(fd_1);

    GEOPM_EXPECT_THROW_MESSAGE(m_msrio_group->push_control("INVALID", geopm::IPlatformTopo::M_DOMAIN_CPU, 0),
                               GEOPM_ERROR_INVALID, "cannot push a control after read_batch() or adjust()");
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

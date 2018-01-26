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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <map>
#include <fstream>
#include "gtest/gtest.h"

#include "geopm_sched.h"
#include "PlatformIO.hpp"
#include "PlatformIOInternal.hpp"
#include "PlatformTopo.hpp"
#include "MSRIO.hpp"
#include "Exception.hpp"
#include "TestPlatformIO.hpp"

class PlatformIOTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();

        geopm::IPlatformIO *m_platform_io;
};

void PlatformIOTest::SetUp()
{
    m_platform_io = new geopm::TestPlatformIO(0x657); // KNL CPUID
}

void PlatformIOTest::TearDown()
{
    delete m_platform_io;
}

TEST_F(PlatformIOTest, whitelist)
{
    std::ifstream file("test/legacy_whitelist.out");
    std::string line;
    uint64_t offset;
    uint64_t  mask;
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

    std::string whitelist = m_platform_io->msr_whitelist();
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
                EXPECT_TRUE(false) << std::setfill('0') << std::hex << "new read offset 0x" << std::setw(8) << offset << " introduced";
            }
            continue;
        }
        uint64_t leg_mask = leg_it->second;
        EXPECT_EQ(mask, mask & leg_mask) << std::setfill('0') << std::hex << "offset 0x" << std::setw(8) << offset << "write mask change detected, from 0x"
            << std::setw(16) << leg_mask << " to 0x" << mask << " bitwise AND yields 0x" << (mask & leg_mask);
    }
}

TEST_F(PlatformIOTest, freq_ctl)
{
    int fd = open("test_msrio_dev_cpu_0_msr_safe",  O_RDWR);
    ASSERT_NE(-1, fd);
    uint64_t value;
    size_t num_read;
    num_read = pread(fd, &value, sizeof(value), 0x0);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x0ULL, value);
    num_read = pread(fd, &value, sizeof(value), 0x198);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x0198019801980198ULL, value);
    num_read = pread(fd, &value, sizeof(value), 0x1A0);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x01A001A001A001A0ULL, value);

    int idx = m_platform_io->push_control("PERF_CTL:FREQ", geopm::IPlatformTopo::M_DOMAIN_CPU, 0);
    ASSERT_EQ(0, idx);
    // Set frequency to 1 GHz
    m_platform_io->adjust(idx, 1e9);
    m_platform_io->write_control();
    num_read = pread(fd, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0xA00ULL, (value & 0xFF00));
    // Set frequency to 5 GHz
    m_platform_io->adjust(idx, 5e9);
    m_platform_io->write_control();
    num_read = pread(fd, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x3200ULL, (value & 0xFF00));
    close(fd);
}

TEST_F(PlatformIOTest, time_signal)
{
    int idx = m_platform_io->push_signal("TIME", geopm::IPlatformTopo::M_DOMAIN_CPU, 0);
    ASSERT_EQ(0, idx);
    std::vector<double> sample(1);
    m_platform_io->read_signal();
    double time_0 = m_platform_io->sample(idx);
    sleep(1);
    m_platform_io->read_signal();
    double time_1 = m_platform_io->sample(idx);
    EXPECT_NEAR(1, time_1 - time_0, 0.1);
    EXPECT_LE(0, time_0);
    EXPECT_LE(0, time_1);
}

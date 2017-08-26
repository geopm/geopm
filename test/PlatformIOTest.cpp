/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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
#include <sstream>
#include "gtest/gtest.h"

#include "geopm_sched.h"
#include "PlatformIO.hpp"
#include "PlatformIOInternal.hpp"
#include "PlatformTopology.hpp"
#include "MSRIO.hpp"
#include "Exception.hpp"

class TestPlatformIO : public geopm::PlatformIO
{
    public:
        TestPlatformIO(int cpuid);
        virtual ~TestPlatformIO();
    protected:
        int cpuid(void);

        int m_cpuid;
};

class PlatformIOTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();

        geopm::IPlatformIO *m_platform_io;
};

namespace geopm
{
    class TestPlatformIOMSRIO : public MSRIO
    {
        public:
            TestPlatformIOMSRIO();
            virtual ~TestPlatformIOMSRIO();
        protected:
            void msr_path(int cpu_idx,
                          bool is_fallback,
                          std::string &path);
            void msr_batch_path(std::string &path);

            std::vector<std::string> m_test_dev_path;
    };

    TestPlatformIOMSRIO::TestPlatformIOMSRIO()
    {
        const size_t MAX_OFFSET = 4096;
        int num_cpu = geopm_sched_num_cpu();
        for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
            std::ostringstream path;
            path << "test_msrio_dev_cpu_" << cpu_idx << "_msr_safe";
            m_test_dev_path.push_back(path.str());
        }

        union field_u {
            uint64_t field;
            uint16_t off[4];
        };
        union field_u fu;
        for (auto &path : m_test_dev_path) {
            int fd = open(path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            int err = ftruncate(fd, MAX_OFFSET);
            if (err) {
                throw Exception("TestMSRIO: ftruncate failed", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            uint64_t *contents = (uint64_t *)mmap(NULL, MAX_OFFSET, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            close(fd);
            size_t num_field = MAX_OFFSET / sizeof(uint64_t);
            for (size_t field_idx = 0; field_idx < num_field; ++field_idx) {
                uint16_t offset = field_idx * sizeof(uint64_t);
                for (int off_idx = 0; off_idx < 4; ++off_idx) {
                   fu.off[off_idx] = offset;
                }
                contents[field_idx] = fu.field;
            }
            munmap(contents, MAX_OFFSET);
        }
    }

    TestPlatformIOMSRIO::~TestPlatformIOMSRIO()
    {
        for (auto &path : m_test_dev_path) {
            unlink(path.c_str());
        }
    }

    void TestPlatformIOMSRIO::msr_path(int cpu_idx,
                             bool is_fallback,
                             std::string &path)
    {
        path = m_test_dev_path[cpu_idx];
    }

    void TestPlatformIOMSRIO::msr_batch_path(std::string &path)
    {
        path = "test_dev_msr_safe";
    }
}


TestPlatformIO::TestPlatformIO(int cpuid)
    : m_cpuid(cpuid)
{
    m_msrio = new geopm::TestPlatformIOMSRIO;
}

TestPlatformIO::~TestPlatformIO()
{

}

int TestPlatformIO::cpuid(void)
{
    return m_cpuid;
}

void PlatformIOTest::SetUp()
{
    m_platform_io = new TestPlatformIO(0x657); // KNL CPUID
}

void PlatformIOTest::TearDown()
{
    delete m_platform_io;
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

    int idx = m_platform_io->push_control("PERF_CTL:FREQ", geopm::GEOPM_DOMAIN_CPU, 0);
    ASSERT_EQ(0, idx);
    // Set frequency to 1 GHz
    m_platform_io->adjust(std::vector<double>{1e9});
    num_read = pread(fd, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0xA00ULL, (value & 0xFF00));
    // Set frequency to 5 GHz
    m_platform_io->adjust(std::vector<double>{5e9});
    num_read = pread(fd, &value, sizeof(value), 0x199);
    EXPECT_EQ(8ULL, num_read);
    EXPECT_EQ(0x3200ULL, (value & 0xFF00));
    close(fd);
}

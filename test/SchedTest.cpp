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

#include <gtest/gtest.h>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "geopm_sched.h"

extern "C"
{
    int geopm_sched_proc_cpuset_helper(int num_cpu, uint32_t *proc_cpuset, FILE *fid);
}

class SchedTest: public :: testing :: Test
{
    protected:
        void SetUp(void);
        void TearDown(void);
        void check_cpuset(const std::string &cpus_allowed, const std::vector<int> &cpus_allowed_vec);
        std::string m_status_path;
        std::string m_status_buffer_0;
        std::string m_status_buffer_1;
};

void SchedTest::SetUp(void)
{
    m_status_path = "geopm_sched_test_status";
    m_status_buffer_0 =
"Name:   cat\n"
"State:  R (running)\n"
"Tgid:   71257\n"
"Ngid:   0\n"
"Pid:    71257\n"
"PPid:   249629\n"
"TracerPid:      0\n"
"Uid:    16003   16003   16003   16003\n"
"Gid:    100     100     100     100\n"
"FDSize: 256\n"
"Groups: 100 1000 \n"
"VmPeak:   107924 kB\n"
"VmSize:   107924 kB\n"
"VmLck:         0 kB\n"
"VmPin:         0 kB\n"
"VmHWM:       616 kB\n"
"VmRSS:       616 kB\n"
"VmData:      180 kB\n"
"VmStk:       144 kB\n"
"VmExe:        44 kB\n"
"VmLib:      1884 kB\n"
"VmPTE:        40 kB\n"
"VmSwap:        0 kB\n"
"Threads:        1\n"
"SigQ:   1/449705\n"
"SigPnd: 0000000000000000\n"
"ShdPnd: 0000000000000000\n"
"SigBlk: 0000000000000000\n"
"SigIgn: 0000000000000000\n"
"SigCgt: 0000000000000000\n"
"CapInh: 0000000000000000\n"
"CapPrm: 0000000000000000\n"
"CapEff: 0000000000000000\n"
"CapBnd: 0000001fffffffff\n"
"Seccomp:        0\n"
"Cpus_allowed:";
    m_status_buffer_1 =
"Cpus_allowed_list:      0-255\n"
"Mems_allowed:   00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000003\n"
"Mems_allowed_list:      0-1\n"
"voluntary_ctxt_switches:        0\n"
"nonvoluntary_ctxt_switches:     1    \n";

}

void SchedTest::TearDown(void)
{
    unlink(m_status_path.c_str());
}

TEST_F(SchedTest, test_proc_cpuset_0)
{
    std::string cpus_allowed("   ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff\n");
    std::ofstream status_file(m_status_path);
    status_file << m_status_buffer_0;
    status_file << cpus_allowed;
    status_file << m_status_buffer_1;
    status_file.close();

    cpu_set_t *cpu_set = CPU_ALLOC(256);
    ASSERT_TRUE(cpu_set != NULL);
    FILE *fid = fopen(m_status_path.c_str(), "r");
    ASSERT_TRUE(fid != NULL);
    int err = geopm_sched_proc_cpuset_helper(256, (uint32_t *)cpu_set, fid);
    ASSERT_TRUE(err == 0);
    fclose(fid);

    for (int i = 0; i < 256; ++i) {
        ASSERT_TRUE(CPU_ISSET(i, cpu_set));
    }

    free(cpu_set);
}

TEST_F(SchedTest, test_proc_cpuset_1)
{
    /*
    numactl --physcpubind=1,17,50,79,87,100,105,126,136,137,157,164,166,168,169,173,174,175,187,189,200,201,209,210,215,219,225,234,235,243 cat /proc/self/status | grep Cpus_allowed:
    Cpus_allowed:   00080c02,08860300,2800e350,20000300,40000210,00808000,00040000,00020002
    */

    std::string cpus_allowed("   00080c02,08860300,2800e350,20000300,40000210,00808000,00040000,00020002\n");
    std::vector<int> cpus_allowed_vec({1, 17, 50, 79, 87, 100, 105, 126, 136, 137, 157, 164, 166, 168, 169,
                                       173, 174, 175, 187, 189, 200, 201, 209, 210, 215, 219, 225, 234, 235, 243});

    check_cpuset(cpus_allowed, cpus_allowed_vec);
}

TEST_F(SchedTest, test_proc_cpuset_2)
{
    /*
    numactl --physcpubind=1,4,8,10,20,30,35,48,53,55,85,86,119,125,132,137,140,151,168,169,170,177,208,213,219,220,236,237,241,248,252 cat /proc/self/status | grep Cpus_allowed:
    Cpus_allowed:   11023000,18210000,00020700,00801210,20800000,00600000,00a10008,40100512
    */

    std::string cpus_allowed("   11023000,18210000,00020700,00801210,20800000,00600000,00a10008,40100512\n");
    std::vector<int> cpus_allowed_vec({1, 4, 8, 10, 20, 30, 35, 48, 53, 55, 85, 86, 119, 125, 132, 137, 140,
                                       151, 168, 169, 170, 177, 208, 213, 219, 220, 236, 237, 241, 248, 252});

    check_cpuset(cpus_allowed, cpus_allowed_vec);
}

TEST_F(SchedTest, test_proc_cpuset_3)
{
    /*
    numactl --physcpubind=0 cat /proc/self/status | grep 'Cpus_allowed:'
    Cpus_allowed:   00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000001
    */

    std::string cpus_allowed("   00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000001\n");
    std::vector<int> cpus_allowed_vec({0});

    check_cpuset(cpus_allowed, cpus_allowed_vec);
}

TEST_F(SchedTest, test_proc_cpuset_4)
{
    /*
    numactl --physcpubind=0-15 cat /proc/self/status | grep 'Cpus_allowed:'
    Cpus_allowed:   00000000,00000000,00000000,00000000,00000000,00000000,00000000,0000ffff
    */

    std::string cpus_allowed("   00000000,00000000,00000000,00000000,00000000,00000000,00000000,0000ffff\n");
    std::vector<int> cpus_allowed_vec({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});

    check_cpuset(cpus_allowed, cpus_allowed_vec);
}


TEST_F(SchedTest, test_proc_cpuset_5)
{
    /*
    numactl --physcpubind=0-31 cat /proc/self/status | grep 'Cpus_allowed:'
    Cpus_allowed:   00000000,00000000,00000000,00000000,00000000,00000000,00000000,ffffffff
    */

    std::string cpus_allowed("   00000000,00000000,00000000,00000000,00000000,00000000,00000000,ffffffff\n");
    std::vector<int> cpus_allowed_vec({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                       16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31});
    check_cpuset(cpus_allowed, cpus_allowed_vec);
}

TEST_F(SchedTest, test_proc_cpuset_6)
{
    /*
    numactl --physcpubind=240-255 cat /proc/self/status | grep 'Cpus_allowed:'
    Cpus_allowed:   ffff0000,00000000,00000000,00000000,00000000,00000000,00000000,00000000
    */

    std::string cpus_allowed("   ffff0000,00000000,00000000,00000000,00000000,00000000,00000000,00000000\n");
    std::vector<int> cpus_allowed_vec({240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255});

    check_cpuset(cpus_allowed, cpus_allowed_vec);
}

TEST_F(SchedTest, test_proc_cpuset_7)
{
    /*
    numactl --physcpubind=224-255 cat /proc/self/status | grep 'Cpus_allowed:'
    Cpus_allowed:   ffffffff,00000000,00000000,00000000,00000000,00000000,00000000,00000000
    */

    std::string cpus_allowed("   ffffffff,00000000,00000000,00000000,00000000,00000000,00000000,00000000\n");
    std::vector<int> cpus_allowed_vec({224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
                                       240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255});

    check_cpuset(cpus_allowed, cpus_allowed_vec);
}

TEST_F(SchedTest, test_proc_cpuset_8)
{
    std::string cpus_allowed("   00000000,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff\n");
    std::ofstream status_file(m_status_path);
    status_file << m_status_buffer_0;
    status_file << cpus_allowed;
    status_file << m_status_buffer_1;
    status_file.close();

    cpu_set_t *cpu_set = CPU_ALLOC(256);
    ASSERT_TRUE(cpu_set != NULL);
    FILE *fid = fopen(m_status_path.c_str(), "r");
    ASSERT_TRUE(fid != NULL);
    int err = geopm_sched_proc_cpuset_helper(256, (uint32_t *)cpu_set, fid);
    ASSERT_TRUE(err == 0);
    fclose(fid);

    for (int i = 0; i < 256; ++i) {
        ASSERT_TRUE(CPU_ISSET(i, cpu_set));
    }

    free(cpu_set);
}

void SchedTest::check_cpuset(const std::string &cpus_allowed, const std::vector<int> &cpus_allowed_vec)
{
    std::ofstream status_file(m_status_path);
    status_file << m_status_buffer_0;
    status_file << cpus_allowed;
    status_file << m_status_buffer_1;
    status_file.close();

    cpu_set_t *cpu_set = CPU_ALLOC(256);
    ASSERT_TRUE(cpu_set != NULL);
    FILE *fid = fopen(m_status_path.c_str(), "r");
    ASSERT_TRUE(fid != NULL);
    int err = geopm_sched_proc_cpuset_helper(256, (uint32_t *)cpu_set, fid);
    ASSERT_TRUE(err == 0);
    fclose(fid);

    unsigned j = 0;
    for (int i = 0; i < 256; ++i) {
        if (j < cpus_allowed_vec.size() && i == cpus_allowed_vec[j]) {
            ASSERT_TRUE(CPU_ISSET(i, cpu_set));
            ++j;
        }
        else {
            ASSERT_TRUE(!CPU_ISSET(i, cpu_set));
        }
    }

    free(cpu_set);
}



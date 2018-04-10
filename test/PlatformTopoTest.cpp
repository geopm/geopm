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

#include <unistd.h>
#include <fstream>
#include "gtest/gtest.h"

#include "PlatformTopo.hpp"
#include "Exception.hpp"

using geopm::IPlatformTopo;
using geopm::PlatformTopo;
using geopm::Exception;

class PlatformTopoTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void write_lscpu(const std::string &lscpu_str);
        std::string m_lscpu_file_name;
        std::string m_hsw_lscpu_str;
        std::string m_knl_lscpu_str;
        std::string m_bdx_lscpu_str;
        bool m_do_unlink;
};

void PlatformTopoTest::SetUp()
{
    m_lscpu_file_name = "PlatformTopoTest-lscpu";
    m_hsw_lscpu_str =
        "Architecture:          x86_64\n"
        "CPU op-mode(s):        32-bit, 64-bit\n"
        "Byte Order:            Little Endian\n"
        "CPU(s):                2\n"
        "On-line CPU(s) mask:   0x3\n"
        "Thread(s) per core:    1\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             1\n"
        "NUMA node(s):          1\n"
        "Vendor ID:             GenuineIntel\n"
        "CPU family:            6\n"
        "Model:                 61\n"
        "Model name:            Intel(R) Core(TM) i7-5650U CPU @ 2.20GHz\n"
        "Stepping:              4\n"
        "CPU MHz:               2200.000\n"
        "BogoMIPS:              4400.00\n"
        "Hypervisor vendor:     KVM\n"
        "Virtualization type:   full\n"
        "L1d cache:             32K\n"
        "L1i cache:             32K\n"
        "L2 cache:              256K\n"
        "L3 cache:              4096K\n"
        "NUMA node0 CPU(s):     0x3\n";
    m_knl_lscpu_str =
        "Architecture:          x86_64\n"
        "CPU op-mode(s):        32-bit, 64-bit\n"
        "Byte Order:            Little Endian\n"
        "CPU(s):                256\n"
        "On-line CPU(s) mask:   0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\n"
        "Thread(s) per core:    4\n"
        "Core(s) per socket:    64\n"
        "Socket(s):             1\n"
        "NUMA node(s):          2\n"
        "Vendor ID:             GenuineIntel\n"
        "CPU family:            6\n"
        "Model:                 87\n"
        "Model name:            Intel(R) Genuine Intel(R) CPU 0000 @ 1.30GHz\n"
        "Stepping:              1\n"
        "CPU MHz:               1030.402\n"
        "BogoMIPS:              2593.93\n"
        "L1d cache:             32K\n"
        "L1i cache:             32K\n"
        "L2 cache:              1024K\n"
        "NUMA node0 CPU(s):     0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\n"
        "NUMA node1 CPU(s):     0x0\n";
    m_bdx_lscpu_str =
        "Architecture:          x86_64\n"
        "CPU op-mode(s):        32-bit, 64-bit\n"
        "Byte Order:            Little Endian\n"
        "CPU(s):                72\n"
        "On-line CPU(s) mask:   0xffffffffffffffffff\n"
        "Thread(s) per core:    2\n"
        "Core(s) per socket:    18\n"
        "Socket(s):             2\n"
        "NUMA node(s):          2\n"
        "Vendor ID:             GenuineIntel\n"
        "CPU family:            6\n"
        "Model:                 79\n"
        "Model name:            Intel(R) Xeon(R) CPU E5-2695 v4 @ 2.10GHz\n"
        "Stepping:              1\n"
        "CPU MHz:               2101.000\n"
        "CPU max MHz:           2101.0000\n"
        "CPU min MHz:           1200.0000\n"
        "BogoMIPS:              4190.38\n"
        "Virtualization:        VT-x\n"
        "L1d cache:             32K\n"
        "L1i cache:             32K\n"
        "L2 cache:              256K\n"
        "L3 cache:              46080K\n"
        "NUMA node0 CPU(s):     0x3ffff00003ffff\n"
        "NUMA node1 CPU(s):     0xffffc0000ffffc0000\n"
        "Flags:                 fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 fma cx16 xtpr pdcm pcid dca sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch epb cat_l3 cdp_l3 invpcid_single intel_pt spec_ctrl ibpb_support tpr_shadow vnmi flexpriority ept vpid fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 erms invpcid rtm cqm rdt_a rdseed adx smap xsaveopt cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local dtherm ida arat pln pts\n";
    m_do_unlink = false;
}

void PlatformTopoTest::TearDown()
{
    if (m_do_unlink) {
        unlink(m_lscpu_file_name.c_str());
    }
}

void PlatformTopoTest::write_lscpu(const std::string &lscpu_str)
{
    std::ofstream lscpu_fid(m_lscpu_file_name);
    lscpu_fid << lscpu_str;
    lscpu_fid.close();
    m_do_unlink = true;
}

TEST_F(PlatformTopoTest, hsw_num_domain)
{
    write_lscpu(m_hsw_lscpu_str);
    geopm::PlatformTopo topo(m_lscpu_file_name);
    EXPECT_EQ(1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_EQ(1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_EQ(2, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_CORE));
    EXPECT_EQ(2, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_CPU));
    EXPECT_EQ(1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD_MEMORY));
    EXPECT_EQ(0, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE_MEMORY));

    // expect same errors on every platform
    /// @todo when implemented, add tests for each platform
    EXPECT_THROW(topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD_NIC), geopm::Exception);
    EXPECT_THROW(topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE_NIC), geopm::Exception);
    EXPECT_THROW(topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD_ACCELERATOR), geopm::Exception);
    EXPECT_THROW(topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE_ACCELERATOR), geopm::Exception);
    EXPECT_THROW(topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_INVALID), geopm::Exception);
}

TEST_F(PlatformTopoTest, knl_num_domain)
{
    write_lscpu(m_knl_lscpu_str);
    geopm::PlatformTopo topo(m_lscpu_file_name);
    EXPECT_EQ(1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_EQ(1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_EQ(64, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_CORE));
    EXPECT_EQ(256, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_CPU));
    EXPECT_EQ(1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD_MEMORY));
    EXPECT_EQ(1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE_MEMORY));
}

TEST_F(PlatformTopoTest, bdx_num_domain)
{
    write_lscpu(m_bdx_lscpu_str);
    geopm::PlatformTopo topo(m_lscpu_file_name);
    EXPECT_EQ(1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_EQ(2, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_EQ(36, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_CORE));
    EXPECT_EQ(72, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_CPU));
    EXPECT_EQ(2, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD_MEMORY));
    EXPECT_EQ(0, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE_MEMORY));
}

TEST_F(PlatformTopoTest, construction)
{
    geopm::PlatformTopo topo;
    EXPECT_EQ(1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_LT(0, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_LT(0, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_CORE));
    EXPECT_LT(0, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_CPU));
    EXPECT_LT(0, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD_MEMORY));
    EXPECT_LT(-1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE_MEMORY));
}

TEST_F(PlatformTopoTest, singleton_construction)
{
    geopm::IPlatformTopo &topo = geopm::platform_topo();
    EXPECT_EQ(1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_LT(0, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_LT(0, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_CORE));
    EXPECT_LT(0, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_CPU));
    EXPECT_LT(0, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_BOARD_MEMORY));
    EXPECT_LT(-1, topo.num_domain(geopm::IPlatformTopo::M_DOMAIN_PACKAGE_MEMORY));
}

TEST_F(PlatformTopoTest, bdx_domain_idx)
{
    write_lscpu(m_bdx_lscpu_str);
    geopm::PlatformTopo topo(m_lscpu_file_name);
    EXPECT_EQ(0, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_BOARD, 0));
    EXPECT_EQ(0, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_PACKAGE, 0));
    EXPECT_EQ(1, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_PACKAGE, 18));
    EXPECT_EQ(0, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_PACKAGE, 9));
    EXPECT_EQ(1, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_PACKAGE, 27));
    EXPECT_EQ(0, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_CORE, 0));
    EXPECT_EQ(17, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_CORE, 17));
    EXPECT_EQ(17, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_CORE, 53));
    EXPECT_EQ(18, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_CORE, 18));
    EXPECT_EQ(18, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_CORE, 54));
    EXPECT_EQ(18, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_CORE, 54));
    for (int cpu_idx = 0; cpu_idx < 72; ++cpu_idx) {
        EXPECT_EQ(cpu_idx, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_CPU, cpu_idx));
    }
    EXPECT_THROW(topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_CPU, 72), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_CPU, 90), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_CPU, -18), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_INVALID, 0), geopm::Exception);

    std::set<int> cpu_set_node0 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                                   36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};
    std::set<int> cpu_set_node1 = {18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
                                   54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71};
    for (auto cpu_idx : cpu_set_node0) {
        EXPECT_EQ(0, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_BOARD_MEMORY, cpu_idx));
    }
    for (auto cpu_idx : cpu_set_node1) {
        EXPECT_EQ(1, topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_BOARD_MEMORY, cpu_idx));
    }
    EXPECT_THROW(topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_PACKAGE_MEMORY, 0), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_BOARD_NIC, 0), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_PACKAGE_NIC, 0), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_BOARD_ACCELERATOR, 0), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(geopm::IPlatformTopo::M_DOMAIN_PACKAGE_ACCELERATOR, 0), geopm::Exception);
}

TEST_F(PlatformTopoTest, bdx_domain_cpus)
{
    write_lscpu(m_bdx_lscpu_str);
    geopm::PlatformTopo topo(m_lscpu_file_name);
    std::set<int> cpu_set_board;
    std::set<int> cpu_set_socket[2];
    cpu_set_socket[0] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17,
                         36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};
    cpu_set_socket[1] = {18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
                         54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71};
    cpu_set_board = cpu_set_socket[0];
    cpu_set_board.insert(cpu_set_socket[1].begin(), cpu_set_socket[1].end());
    std::set<int> cpu_set_expect;
    std::set<int> cpu_set_actual;
    topo.domain_cpus(IPlatformTopo::M_DOMAIN_BOARD, 0, cpu_set_actual);
    EXPECT_EQ(cpu_set_board, cpu_set_actual);

    topo.domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE, 0, cpu_set_actual);
    EXPECT_EQ(cpu_set_socket[0], cpu_set_actual);

    topo.domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE, 1, cpu_set_actual);
    EXPECT_EQ(cpu_set_socket[1], cpu_set_actual);

    cpu_set_expect = {0, 36};
    topo.domain_cpus(IPlatformTopo::M_DOMAIN_CORE, 0, cpu_set_actual);
    EXPECT_EQ(cpu_set_expect, cpu_set_actual);

    cpu_set_expect = {1, 37};
    topo.domain_cpus(IPlatformTopo::M_DOMAIN_CORE, 1, cpu_set_actual);
    EXPECT_EQ(cpu_set_expect, cpu_set_actual);

    cpu_set_expect = {0};
    topo.domain_cpus(IPlatformTopo::M_DOMAIN_CPU, 0, cpu_set_actual);
    EXPECT_EQ(cpu_set_expect, cpu_set_actual);

    cpu_set_expect = {1};
    topo.domain_cpus(IPlatformTopo::M_DOMAIN_CPU, 1, cpu_set_actual);
    EXPECT_EQ(cpu_set_expect, cpu_set_actual);

    cpu_set_expect = cpu_set_socket[0];
    topo.domain_cpus(IPlatformTopo::M_DOMAIN_BOARD_MEMORY, 0, cpu_set_actual);
    EXPECT_EQ(cpu_set_expect, cpu_set_actual);

    cpu_set_expect = cpu_set_socket[1];
    topo.domain_cpus(IPlatformTopo::M_DOMAIN_BOARD_MEMORY, 1, cpu_set_actual);
    EXPECT_EQ(cpu_set_expect, cpu_set_actual);

    // TODO: still to be implemented
    EXPECT_THROW(topo.domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE_MEMORY, 0, cpu_set_actual), Exception);
    EXPECT_THROW(topo.domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE_ACCELERATOR, 0, cpu_set_actual), Exception);
    EXPECT_THROW(topo.domain_cpus(IPlatformTopo::M_DOMAIN_PACKAGE_NIC, 0, cpu_set_actual), Exception);
    EXPECT_THROW(topo.domain_cpus(IPlatformTopo::M_DOMAIN_BOARD_NIC, 0, cpu_set_actual), Exception);
    EXPECT_THROW(topo.domain_cpus(IPlatformTopo::M_DOMAIN_BOARD_ACCELERATOR, 0, cpu_set_actual), Exception);

}

TEST_F(PlatformTopoTest, bdx_is_domain_within)
{
    write_lscpu(m_bdx_lscpu_str);
    geopm::PlatformTopo topo(m_lscpu_file_name);

    // domains containing CPUs
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_CPU, IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_CPU, IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_CPU, IPlatformTopo::M_DOMAIN_CORE));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_CPU, IPlatformTopo::M_DOMAIN_CPU));
    // needed to support POWER_DRAM signal
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_CPU, IPlatformTopo::M_DOMAIN_BOARD_MEMORY));

    // domains containing cores
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_CORE, IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_CORE, IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_CORE, IPlatformTopo::M_DOMAIN_CORE));
    EXPECT_FALSE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_CORE, IPlatformTopo::M_DOMAIN_CPU));

    // domains containing package
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_PACKAGE, IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_PACKAGE, IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_FALSE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_PACKAGE, IPlatformTopo::M_DOMAIN_CORE));
    EXPECT_FALSE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_PACKAGE, IPlatformTopo::M_DOMAIN_CPU));

    // domains containing board
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_BOARD, IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_FALSE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_BOARD, IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_FALSE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_BOARD, IPlatformTopo::M_DOMAIN_CORE));
    EXPECT_FALSE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_BOARD, IPlatformTopo::M_DOMAIN_CPU));

    // other domains in the board
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_BOARD_NIC, IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_BOARD_ACCELERATOR, IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_BOARD_MEMORY, IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_PACKAGE_NIC, IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_PACKAGE_ACCELERATOR, IPlatformTopo::M_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_PACKAGE_MEMORY, IPlatformTopo::M_DOMAIN_BOARD));

    // other domains in the package
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_PACKAGE_NIC, IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_PACKAGE_ACCELERATOR, IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_TRUE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_PACKAGE_MEMORY, IPlatformTopo::M_DOMAIN_PACKAGE));

    // TODO: remove resource manager domain
    EXPECT_FALSE(topo.is_domain_within(IPlatformTopo::M_DOMAIN_RESOURCE_MANAGER, IPlatformTopo::M_DOMAIN_BOARD));

}

TEST_F(PlatformTopoTest, parse_error)
{
    std::string lscpu_missing_cpu =
        "Thread(s) per core:    1\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             1\n"
        "NUMA node(s):          1\n";
    std::string lscpu_missing_thread =
        "CPU(s):                2\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             1\n"
        "NUMA node(s):          1\n";
    std::string lscpu_missing_cores =
        "CPU(s):                2\n"
        "Thread(s) per core:    1\n"
        "Socket(s):             1\n"
        "NUMA node(s):          1\n";
    std::string lscpu_missing_sockets =
        "CPU(s):                2\n"
        "Thread(s) per core:    1\n"
        "Core(s) per socket:    2\n"
        "NUMA node(s):          1\n";
    std::string lscpu_missing_numa =
        "CPU(s):                2\n"
        "Thread(s) per core:    1\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             1\n";

    write_lscpu(lscpu_missing_cpu);
    EXPECT_THROW(PlatformTopo topo(m_lscpu_file_name), Exception);
    write_lscpu(lscpu_missing_thread);
    EXPECT_THROW(PlatformTopo topo(m_lscpu_file_name), Exception);
    write_lscpu(lscpu_missing_cores);
    EXPECT_THROW(PlatformTopo topo(m_lscpu_file_name), Exception);
    write_lscpu(lscpu_missing_sockets);
    EXPECT_THROW(PlatformTopo topo(m_lscpu_file_name), Exception);
    write_lscpu(lscpu_missing_numa);
    EXPECT_THROW(PlatformTopo topo(m_lscpu_file_name), Exception);

    std::string lscpu_non_number =
        "CPU(s):                xx\n"
        "Thread(s) per core:    1\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             1\n"
        "NUMA node(s):          1\n";
    write_lscpu(lscpu_non_number);
    EXPECT_THROW(PlatformTopo topo(m_lscpu_file_name), Exception);

    std::string lscpu_invalid =
        "CPU(s):                2\n"
        "Thread(s) per core:    2\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             2\n"
        "NUMA node(s):          1\n";
    write_lscpu(lscpu_invalid);
    EXPECT_THROW(PlatformTopo topo(m_lscpu_file_name), Exception);
}

            /////////////////////////////////////////////////////////////////////////////////////////
            // Notes:
            //
            // How do we ask how many sockets per board?
            //    num_domain(M_DOMAIN_PACKAGE) / num_domain(M_DOMAIN_BOARD)
            // How do we ask how many CPUs per socket?
            //    num_domain(M_DOMAIN_CPU) / num_domain(M_DOMAIN_PACKAGE)
            // How do we know which Linux logical CPUs are on core 5?
            //    domain_cpus(M_DOMAIN_CORE, 5, cpu_idx_set);
            // How do we ask if there is on package memory?
            //    num_domain(M_DOMAIN_PACKAGE_MEMORY) > 0
            // How do we ask if the frequency control is per package or per core?
            //    platform_io().control_domain_type("PERF_CTL:FREQ") == M_DOMAIN_PACKAGE
            // How do we ask which socket Linux logical CPU 8 is on?
            //    domain_idx(M_DOMAIN_PACKAGE, 8)
            // How do we find out all of the other Linux logical CPUs that share a socket with CPU 8?
            //    domain_cpus(M_DOMAIN_PACKAGE, domain_idx(M_DOMAIN_PACKAGE, 8), socket_cpu_set)
            // How do we define a group all linux logical CPUs that are divisable by 4?
            //    int num_cpu = num_domain(M_DOMAIN_CPU);
            //    for (int i = 0; i < num_cpu; i +=4) {
            //        domain_idx.push_back(i);
            //    }
            //    uint64_t group_domain = group_ext_define(0, domain_idx);

/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <limits.h>
#include <utime.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>

#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm/Helper.hpp"
#include "MockGPUTopo.hpp"
#include "PlatformTopoImp.hpp"
#include "geopm/Exception.hpp"
#include "config.h"
#include "LevelZero.hpp"
#include "geopm_test.hpp"
#include "geopm_time.h"

using geopm::PlatformTopo;
using geopm::PlatformTopoImp;
using geopm::Exception;
using testing::Return;
using testing::_;

class PlatformTopoTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void write_lscpu(const std::string &lscpu_str);
        void spoof_lscpu(void);
        std::string m_path_env_save;
        std::string m_lscpu_file_name;
        std::string m_hsw_lscpu_str;
        std::string m_knl_lscpu_str;
        std::string m_bdx_lscpu_str;
        std::string m_ppc_lscpu_str;
        std::string m_no0x_lscpu_str;
        std::string m_no_numa_lscpu_str;
        std::string m_spr_lscpu_str;
        std::string m_gpu_str;
        std::string m_gpu_lscpu_str;
        std::string m_lscpu_str;
        bool m_do_unlink;
};

void PlatformTopoTest::spoof_lscpu(void)
{
    // Create script that wraps lscpu
    std::string lscpu_script = "#!/bin/bash\n"
                               "if [ ! -z \"$PLATFORM_TOPO_TEST_LSCPU_ERROR\" ]; then\n"
                               "    exit -1;\n"
                               "else\n"
                               "    echo 'Architecture:          x86_64'\n"
                               "    echo 'CPU op-mode(s):        32-bit, 64-bit'\n"
                               "    echo 'Byte Order:            Little Endian'\n"
                               "    echo 'CPU(s):                2'\n"
                               "    echo 'On-line CPU(s) mask:   0x3'\n"
                               "    echo 'Thread(s) per core:    1'\n"
                               "    echo 'Core(s) per socket:    2'\n"
                               "    echo 'Socket(s):             1'\n"
                               "    echo 'NUMA node(s):          1'\n"
                               "    echo 'Vendor ID:             GenuineIntel'\n"
                               "    echo 'CPU family:            6'\n"
                               "    echo 'Model:                 61'\n"
                               "    echo 'Model name:            Intel(R) Core(TM) i7-5650U CPU @ 2.20GHz'\n"
                               "    echo 'Stepping:              4'\n"
                               "    echo 'CPU MHz:               2200.000'\n"
                               "    echo 'BogoMIPS:              4400.00'\n"
                               "    echo 'Hypervisor vendor:     KVM'\n"
                               "    echo 'Virtualization type:   full'\n"
                               "    echo 'L1d cache:             32K'\n"
                               "    echo 'L1i cache:             32K'\n"
                               "    echo 'L2 cache:              256K'\n"
                               "    echo 'L3 cache:              4096K'\n"
                               "    echo 'NUMA node0 CPU(s):     0x3'\n"
                               "fi\n";
    std::ofstream script_stream("lscpu");
    script_stream << lscpu_script;
    script_stream.close();
    chmod("lscpu", S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);

    // Put CWD in the front of PATH
    std::string path_env(":" + m_path_env_save);
    setenv("PATH", path_env.c_str(), 1);
}


void PlatformTopoTest::SetUp()
{
    const char *path_cstr = getenv("PATH");
    m_path_env_save = path_cstr ? path_cstr : "";
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
    m_ppc_lscpu_str =
        "Architecture:          ppc64le\n"
        "Byte Order:            Little Endian\n"
        "CPU(s):                160\n"
        "On-line CPU(s) mask:   0x101010101010101010101010101010101010101\n"
        "Off-line CPU(s) mask:  0xfefefefefefefefefefefefefefefefefefefefe\n"
        "Thread(s) per core:    1\n"
        "Core(s) per socket:    10\n"
        "Socket(s):             2\n"
        "NUMA node(s):          2\n"
        "Model:                 1.0 (pvr 004c 0100)\n"
        "Model name:            POWER8NVL (raw), altivec supported\n"
        "CPU max MHz:           4023.0000\n"
        "CPU min MHz:           2394.0000\n"
        "Hypervisor vendor:     (null)\n"
        "Virtualization type:   full\n"
        "L1d cache:             64K\n"
        "L1i cache:             32K\n"
        "L2 cache:              512K\n"
        "L3 cache:              8192K\n"
        "NUMA node0 CPU(s):     0x1010101010101010101\n"
        "NUMA node1 CPU(s):     0x101010101010101010100000000000000000000\n";
    m_no0x_lscpu_str =
        "Architecture:          x86_64\n"
        "CPU op-mode(s):        32-bit, 64-bit\n"
        "Byte Order:            Little Endian\n"
        "CPU(s):                72\n"
        "On-line CPU(s) mask:   ffffffffffffffffff\n"
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
        "NUMA node0 CPU(s):     3ffff00003ffff\n"
        "NUMA node1 CPU(s):     ffffc0000ffffc0000\n"
        "Flags:                 fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 fma cx16 xtpr pdcm pcid dca sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch epb cat_l3 cdp_l3 invpcid_single intel_pt spec_ctrl ibpb_support tpr_shadow vnmi flexpriority ept vpid fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 erms invpcid rtm cqm rdt_a rdseed adx smap xsaveopt cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local dtherm ida arat pln pts\n";
    m_no_numa_lscpu_str =
        "Architecture:          x86_64\n"
        "CPU op-mode(s):        32-bit, 64-bit\n"
        "Byte Order:            Little Endian\n"
        "CPU(s):                72\n"
        "On-line CPU(s) mask:   ffffffffffffffffff\n"
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
        "Flags:                 fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 fma cx16 xtpr pdcm pcid dca sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch epb cat_l3 cdp_l3 invpcid_single intel_pt spec_ctrl ibpb_support tpr_shadow vnmi flexpriority ept vpid fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 erms invpcid rtm cqm rdt_a rdseed adx smap xsaveopt cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local dtherm ida arat pln pts\n";
    m_spr_lscpu_str =
        "Architecture:                    x86_64\n"
        "CPU op-mode(s):                  32-bit, 64-bit\n"
        "Byte Order:                      Little Endian\n"
        "Address sizes:                   52 bits physical, 57 bits virtual\n"
        "CPU(s):                          208\n"
        "On-line CPU(s) mask:             ffffffffffffffffffffffffffffffffffffffffffffffffffff\n"
        "Thread(s) per core:              2\n"
        "Core(s) per socket:              52\n"
        "Socket(s):                       2\n"
        "NUMA node(s):                    2\n"
        "Vendor ID:                       GenuineIntel\n"
        "CPU family:                      6\n"
        "Model:                           143\n"
        "Model name:                      Intel(R) Xeon(R) Platinum 8465C CPU @2.10GHz\n"
        "Stepping:                        5\n"
        "Frequency boost:                 enabled\n"
        "CPU MHz:                         3714.500\n"
        "CPU max MHz:                     2101.0000\n"
        "CPU min MHz:                     800.0000\n"
        "BogoMIPS:                        4200.00\n"
        "Virtualization:                  VT-x\n"
        "L1d cache:                       4.9 MiB\n"
        "L1i cache:                       3.3 MiB\n"
        "L2 cache:                        208 MiB\n"
        "L3 cache:                        210 MiB\n"
        "NUMA node0 CPU(s):               fffffffffffff0000000000000fffffffffffff\n"
        "NUMA node1 CPU(s):               fffffffffffff0000000000000fffffffffffff0000000000000\n"
        "Flags:                           fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid dca sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb cat_l3 cat_l2 cdp_l3 invpcid_single cdp_l2 ssbd mba ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid cqm rdt_a avx512f avx512dq rdseed adx smap avx512ifma clflushopt clwb intel_pt avx512cd sha_ni avx512bw avx512vl xsaveopt xsavec xgetbv1 xsaves cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local split_lock_detect avx512_bf16 wbnoinvd dtherm ida arat pln pts hwp hwp_act_window hwp_epp hwp_pkg_req avx512vbmi umip pku ospke waitpkg avx512_vbmi2 gfni vaes vpclmulqdq avx512_vnni avx512_bitalg tme avx512_vpopcntdq rdpid cldemote movdiri movdir64b enqcmd fsrm md_clear serialize tsxldtrk avx512_fp16 flush_l1d arch_capabilities\n";
    m_gpu_str =
        "GPU node0 CPU(s): 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,204\n"
        "GPU node1 CPU(s): 34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,205\n"
        "GPU node2 CPU(s): 68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,206\n"
        "GPU node3 CPU(s): 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,207\n"
        "GPU node4 CPU(s): 136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169\n"
        "GPU node5 CPU(s): 170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203\n"
        "GPU chip0 CPU(s): 0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,204\n"
        "GPU chip1 CPU(s): 1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33\n"
        "GPU chip2 CPU(s): 34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,205\n"
        "GPU chip3 CPU(s): 35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,65,67\n"
        "GPU chip4 CPU(s): 68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,100,206\n"
        "GPU chip5 CPU(s): 69,71,73,75,77,79,81,83,85,87,89,91,93,95,97,99,101\n"
        "GPU chip6 CPU(s): 102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,207\n"
        "GPU chip7 CPU(s): 103,105,107,109,111,113,115,117,119,121,123,125,127,129,131,133,135\n"
        "GPU chip8 CPU(s): 136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168\n"
        "GPU chip9 CPU(s): 137,139,141,143,145,147,149,151,153,155,157,159,161,163,165,167,169\n"
        "GPU chip10 CPU(s): 170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,200,202\n"
        "GPU chip11 CPU(s): 171,173,175,177,179,181,183,185,187,189,191,193,195,197,199,201,203\n";
    m_gpu_lscpu_str = m_spr_lscpu_str + m_gpu_str;
    m_do_unlink = false;
}

void PlatformTopoTest::TearDown()
{
    if (m_do_unlink) {
        unlink(m_lscpu_file_name.c_str());
    }
    (void)unlink("lscpu");
    (void)setenv("PATH", m_path_env_save.c_str(), 1);
    unsetenv("PLATFORM_TOPO_TEST_LSCPU_ERROR");
}

void PlatformTopoTest::write_lscpu(const std::string &lscpu_str)
{
    std::ofstream lscpu_fid(m_lscpu_file_name);
    lscpu_fid << lscpu_str;
    lscpu_fid.close();
    m_do_unlink = true;
    // Set perms to 0o600 to ensure test file is used and not regenerated
    mode_t default_perms = S_IRUSR | S_IWUSR;
    chmod(m_lscpu_file_name.c_str(), default_perms);
}

TEST_F(PlatformTopoTest, hsw_num_domain)
{
    write_lscpu(m_hsw_lscpu_str);
    PlatformTopoImp topo(m_lscpu_file_name);
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_PACKAGE));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_CORE));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_CPU));
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_MEMORY));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY));

    /// @todo when implemented, add tests for each platform
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_NIC));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_GPU));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_GPU_CHIP));

    EXPECT_THROW(topo.num_domain(GEOPM_DOMAIN_INVALID), geopm::Exception);
}

TEST_F(PlatformTopoTest, knl_num_domain)
{
    write_lscpu(m_knl_lscpu_str);
    PlatformTopoImp topo(m_lscpu_file_name);
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_PACKAGE));
    EXPECT_EQ(64, topo.num_domain(GEOPM_DOMAIN_CORE));
    EXPECT_EQ(256, topo.num_domain(GEOPM_DOMAIN_CPU));
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_MEMORY));
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY));
}

TEST_F(PlatformTopoTest, bdx_num_domain)
{
    write_lscpu(m_bdx_lscpu_str);
    PlatformTopoImp topo(m_lscpu_file_name);
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_PACKAGE));
    EXPECT_EQ(36, topo.num_domain(GEOPM_DOMAIN_CORE));
    EXPECT_EQ(72, topo.num_domain(GEOPM_DOMAIN_CPU));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_MEMORY));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_GPU));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_GPU_CHIP));
}

TEST_F(PlatformTopoTest, gpu_num_domain)
{
    write_lscpu(m_gpu_lscpu_str);
    PlatformTopoImp topo(m_lscpu_file_name);
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_PACKAGE));
    EXPECT_EQ(104, topo.num_domain(GEOPM_DOMAIN_CORE));
    EXPECT_EQ(208, topo.num_domain(GEOPM_DOMAIN_CPU));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_MEMORY));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_NIC));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC));
    EXPECT_EQ(6, topo.num_domain(GEOPM_DOMAIN_GPU));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU));
    EXPECT_EQ(12, topo.num_domain(GEOPM_DOMAIN_GPU_CHIP));

    EXPECT_THROW(topo.num_domain(GEOPM_DOMAIN_INVALID), geopm::Exception);
}

TEST_F(PlatformTopoTest, ppc_num_domain)
{
    write_lscpu(m_ppc_lscpu_str);
    PlatformTopoImp topo(m_lscpu_file_name);
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_PACKAGE));
    EXPECT_EQ(20, topo.num_domain(GEOPM_DOMAIN_CORE));
    EXPECT_EQ(20, topo.num_domain(GEOPM_DOMAIN_CPU));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_MEMORY));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY));
}

TEST_F(PlatformTopoTest, no0x_num_domain)
{
    write_lscpu(m_no0x_lscpu_str);
    PlatformTopoImp topo(m_lscpu_file_name);
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_PACKAGE));
    EXPECT_EQ(36, topo.num_domain(GEOPM_DOMAIN_CORE));
    EXPECT_EQ(72, topo.num_domain(GEOPM_DOMAIN_CPU));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_MEMORY));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY));
}

TEST_F(PlatformTopoTest, no_numa_num_domain)
{
    write_lscpu(m_no_numa_lscpu_str);
    PlatformTopoImp topo(m_lscpu_file_name);
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_EQ(2, topo.num_domain(GEOPM_DOMAIN_PACKAGE));
    EXPECT_EQ(36, topo.num_domain(GEOPM_DOMAIN_CORE));
    EXPECT_EQ(72, topo.num_domain(GEOPM_DOMAIN_CPU));
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_MEMORY));
    EXPECT_EQ(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY));
}

TEST_F(PlatformTopoTest, construction)
{
    PlatformTopoImp topo;
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_LT(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE));
    EXPECT_LT(0, topo.num_domain(GEOPM_DOMAIN_CORE));
    EXPECT_LT(0, topo.num_domain(GEOPM_DOMAIN_CPU));
    EXPECT_LT(0, topo.num_domain(GEOPM_DOMAIN_MEMORY));
    EXPECT_LT(-1, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY));
}

TEST_F(PlatformTopoTest, singleton_construction)
{
    const PlatformTopo &topo = geopm::platform_topo();
    EXPECT_EQ(1, topo.num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_LT(0, topo.num_domain(GEOPM_DOMAIN_PACKAGE));
    EXPECT_LT(0, topo.num_domain(GEOPM_DOMAIN_CORE));
    EXPECT_LT(0, topo.num_domain(GEOPM_DOMAIN_CPU));
    EXPECT_LT(0, topo.num_domain(GEOPM_DOMAIN_MEMORY));
    EXPECT_LT(-1, topo.num_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY));
}

TEST_F(PlatformTopoTest, bdx_domain_idx)
{
    write_lscpu(m_bdx_lscpu_str);
    PlatformTopoImp topo(m_lscpu_file_name);
    EXPECT_EQ(0, topo.domain_idx(GEOPM_DOMAIN_BOARD, 0));
    EXPECT_EQ(0, topo.domain_idx(GEOPM_DOMAIN_PACKAGE, 0));
    EXPECT_EQ(1, topo.domain_idx(GEOPM_DOMAIN_PACKAGE, 18));
    EXPECT_EQ(0, topo.domain_idx(GEOPM_DOMAIN_PACKAGE, 9));
    EXPECT_EQ(1, topo.domain_idx(GEOPM_DOMAIN_PACKAGE, 27));
    EXPECT_EQ(0, topo.domain_idx(GEOPM_DOMAIN_CORE, 0));
    EXPECT_EQ(17, topo.domain_idx(GEOPM_DOMAIN_CORE, 17));
    EXPECT_EQ(17, topo.domain_idx(GEOPM_DOMAIN_CORE, 53));
    EXPECT_EQ(18, topo.domain_idx(GEOPM_DOMAIN_CORE, 18));
    EXPECT_EQ(18, topo.domain_idx(GEOPM_DOMAIN_CORE, 54));
    EXPECT_EQ(18, topo.domain_idx(GEOPM_DOMAIN_CORE, 54));
    for (int cpu_idx = 0; cpu_idx < 72; ++cpu_idx) {
        EXPECT_EQ(cpu_idx, topo.domain_idx(GEOPM_DOMAIN_CPU, cpu_idx));
    }
    EXPECT_THROW(topo.domain_idx(GEOPM_DOMAIN_CPU, 72), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(GEOPM_DOMAIN_CPU, 90), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(GEOPM_DOMAIN_CPU, -18), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(GEOPM_DOMAIN_INVALID, 0), geopm::Exception);

    std::set<int> cpu_set_node0 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                                   36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};
    std::set<int> cpu_set_node1 = {18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
                                   54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71};
    for (auto cpu_idx : cpu_set_node0) {
        EXPECT_EQ(0, topo.domain_idx(GEOPM_DOMAIN_MEMORY, cpu_idx));
    }
    for (auto cpu_idx : cpu_set_node1) {
        EXPECT_EQ(1, topo.domain_idx(GEOPM_DOMAIN_MEMORY, cpu_idx));
    }

    EXPECT_THROW(topo.domain_idx(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY, 0), geopm::Exception);

    EXPECT_THROW(topo.domain_idx(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY, 0), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(GEOPM_DOMAIN_NIC, 0), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC, 0), geopm::Exception);
    EXPECT_THROW(topo.domain_idx(GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU, 0), geopm::Exception);
}

TEST_F(PlatformTopoTest, bdx_is_nested_domain)
{
    write_lscpu(m_bdx_lscpu_str);
    PlatformTopoImp topo(m_lscpu_file_name);

    // domains containing CPUs
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_PACKAGE));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_CORE));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_CPU));
    // needed to support DRAM_POWER signal
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_MEMORY));

    // domains containing cores
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_PACKAGE));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_CORE));
    EXPECT_FALSE(topo.is_nested_domain(GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_CPU));

    // domains containing package
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_PACKAGE));
    EXPECT_FALSE(topo.is_nested_domain(GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_CORE));
    EXPECT_FALSE(topo.is_nested_domain(GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_CPU));

    // domains containing board
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_BOARD, GEOPM_DOMAIN_BOARD));
    EXPECT_FALSE(topo.is_nested_domain(GEOPM_DOMAIN_BOARD, GEOPM_DOMAIN_PACKAGE));
    EXPECT_FALSE(topo.is_nested_domain(GEOPM_DOMAIN_BOARD, GEOPM_DOMAIN_CORE));
    EXPECT_FALSE(topo.is_nested_domain(GEOPM_DOMAIN_BOARD, GEOPM_DOMAIN_CPU));

    // other domains in the board
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_NIC, GEOPM_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_GPU, GEOPM_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_MEMORY, GEOPM_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC, GEOPM_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU, GEOPM_DOMAIN_BOARD));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY, GEOPM_DOMAIN_BOARD));

    // other domains in the package
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC, GEOPM_DOMAIN_PACKAGE));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU, GEOPM_DOMAIN_PACKAGE));
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY, GEOPM_DOMAIN_PACKAGE));

    /// GPU chip is a subdomain of the GPU
    EXPECT_TRUE(topo.is_nested_domain(GEOPM_DOMAIN_GPU_CHIP, GEOPM_DOMAIN_GPU));
}

TEST_F(PlatformTopoTest, bdx_domain_nested)
{
    write_lscpu(m_bdx_lscpu_str);
    PlatformTopoImp topo(m_lscpu_file_name);

    std::set<int> cpu_set_board;
    std::set<int> core_set_board;
    std::set<int> cpu_set_socket[2];
    std::set<int> core_set_socket[2];
    core_set_socket[0] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17};
    cpu_set_socket[0] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17,
                         36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};
    core_set_socket[1] = {18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};
    cpu_set_socket[1] = {18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
                         54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71};
    cpu_set_board = cpu_set_socket[0];
    cpu_set_board.insert(cpu_set_socket[1].begin(), cpu_set_socket[1].end());
    core_set_board = core_set_socket[0];
    core_set_board.insert(core_set_socket[1].begin(), core_set_socket[1].end());

    // CPUs
    std::set<int> idx_set_expect;
    std::set<int> idx_set_actual;
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CPU,
                                        GEOPM_DOMAIN_BOARD, 0);
    EXPECT_EQ(cpu_set_board, idx_set_actual);

    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CPU,
                                        GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(cpu_set_socket[0], idx_set_actual);

    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CPU,
                                        GEOPM_DOMAIN_PACKAGE, 1);
    EXPECT_EQ(cpu_set_socket[1], idx_set_actual);

    idx_set_expect = {0, 36};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CPU,
                                        GEOPM_DOMAIN_CORE, 0);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    idx_set_expect = {1, 37};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CPU,
                                        GEOPM_DOMAIN_CORE, 1);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    idx_set_expect = {0};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CPU,
                                        GEOPM_DOMAIN_CPU, 0);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    idx_set_expect = {1};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CPU,
                                        GEOPM_DOMAIN_CPU, 1);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    idx_set_expect = cpu_set_socket[0];
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CPU,
                                        GEOPM_DOMAIN_MEMORY, 0);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    idx_set_expect = cpu_set_socket[1];
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CPU,
                                        GEOPM_DOMAIN_MEMORY, 1);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    // Core
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CORE,
                                        GEOPM_DOMAIN_BOARD, 0);
    EXPECT_EQ(core_set_board, idx_set_actual);

    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CORE,
                                        GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(core_set_socket[0], idx_set_actual);

    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CORE,
                                        GEOPM_DOMAIN_PACKAGE, 1);
    EXPECT_EQ(core_set_socket[1], idx_set_actual);

    idx_set_expect = {0};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CORE,
                                        GEOPM_DOMAIN_CORE, 0);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    idx_set_expect = {1};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_CORE,
                                        GEOPM_DOMAIN_CORE, 1);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    EXPECT_THROW(topo.domain_nested(GEOPM_DOMAIN_CORE,
                                    GEOPM_DOMAIN_CPU, 0),
                 Exception);

    // Package
    idx_set_expect = {0, 1};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_PACKAGE,
                                        GEOPM_DOMAIN_BOARD, 0);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    idx_set_expect = {0};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_PACKAGE,
                                        GEOPM_DOMAIN_PACKAGE, 0);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    idx_set_expect = {1};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_PACKAGE,
                                        GEOPM_DOMAIN_PACKAGE, 1);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    EXPECT_THROW(topo.domain_nested(GEOPM_DOMAIN_PACKAGE,
                                    GEOPM_DOMAIN_CPU, 0),
                 Exception);

    // Board Memory
    idx_set_expect = {0, 1};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_MEMORY,
                                        GEOPM_DOMAIN_BOARD, 0);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    idx_set_expect = {0};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_MEMORY,
                                        GEOPM_DOMAIN_MEMORY, 0);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    idx_set_expect = {1};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_MEMORY,
                                        GEOPM_DOMAIN_MEMORY, 1);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    EXPECT_THROW(topo.domain_nested(GEOPM_DOMAIN_MEMORY,
                                    GEOPM_DOMAIN_CPU, 0),
                 Exception);

    // Board
    idx_set_expect = {0};
    idx_set_actual = topo.domain_nested(GEOPM_DOMAIN_BOARD,
                                        GEOPM_DOMAIN_BOARD, 0);
    EXPECT_EQ(idx_set_expect, idx_set_actual);

    // TODO: still to be implemented
    EXPECT_THROW(topo.domain_nested(GEOPM_DOMAIN_CPU,
                                    GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY, 0), Exception);
    EXPECT_THROW(topo.domain_nested(GEOPM_DOMAIN_CPU,
                                    GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU, 0), Exception);
    EXPECT_THROW(topo.domain_nested(GEOPM_DOMAIN_CPU,
                                    GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC, 0), Exception);
    EXPECT_THROW(topo.domain_nested(GEOPM_DOMAIN_CPU,
                                    GEOPM_DOMAIN_NIC, 0), Exception);
}

TEST_F(PlatformTopoTest, parse_error)
{
    std::string lscpu_missing_cpu =
        "Thread(s) per core:    1\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             1\n"
        "NUMA node(s):          1\n"
        "On-line CPU(s) mask:   0x1\n";
    std::string lscpu_missing_thread =
        "CPU(s):                2\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             1\n"
        "NUMA node(s):          1\n"
        "On-line CPU(s) mask:   0x1\n";
    std::string lscpu_missing_cores =
        "CPU(s):                2\n"
        "Thread(s) per core:    1\n"
        "Socket(s):             1\n"
        "NUMA node(s):          1\n"
        "On-line CPU(s) mask:   0x1\n";
    std::string lscpu_missing_sockets =
        "CPU(s):                2\n"
        "Thread(s) per core:    1\n"
        "Core(s) per socket:    2\n"
        "NUMA node(s):          1\n"
        "On-line CPU(s) mask:   0x1\n";
    std::string lscpu_missing_numa =
        "CPU(s):                2\n"
        "Thread(s) per core:    1\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             1\n"
        "On-line CPU(s) mask:   0x1\n";

    write_lscpu(lscpu_missing_cpu);
    EXPECT_THROW(PlatformTopoImp topo(m_lscpu_file_name), Exception);
    write_lscpu(lscpu_missing_thread);
    EXPECT_THROW(PlatformTopoImp topo(m_lscpu_file_name), Exception);
    write_lscpu(lscpu_missing_cores);
    EXPECT_THROW(PlatformTopoImp topo(m_lscpu_file_name), Exception);
    write_lscpu(lscpu_missing_sockets);
    EXPECT_THROW(PlatformTopoImp topo(m_lscpu_file_name), Exception);
    write_lscpu(lscpu_missing_numa);
    PlatformTopoImp topo(m_lscpu_file_name);

    std::string lscpu_non_number =
        "CPU(s):                xx\n"
        "Thread(s) per core:    1\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             1\n"
        "NUMA node(s):          1\n";
    write_lscpu(lscpu_non_number);
    EXPECT_THROW(PlatformTopoImp topo(m_lscpu_file_name), Exception);

    std::string lscpu_invalid =
        "CPU(s):                2\n"
        "Thread(s) per core:    2\n"
        "Core(s) per socket:    2\n"
        "Socket(s):             2\n"
        "NUMA node(s):          1\n";
    write_lscpu(lscpu_invalid);
    EXPECT_THROW(PlatformTopoImp topo(m_lscpu_file_name), Exception);
}

TEST_F(PlatformTopoTest, domain_type_to_name)
{
    EXPECT_THROW(PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_INVALID),
                 Exception);

    EXPECT_EQ("board", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_BOARD));
    EXPECT_EQ("package", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_PACKAGE));
    EXPECT_EQ("core", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_CORE));
    EXPECT_EQ("cpu", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_CPU));
    EXPECT_EQ("memory", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_MEMORY));
    EXPECT_EQ("package_integrated_memory", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY));
    EXPECT_EQ("nic", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_NIC));
    EXPECT_EQ("package_integrated_nic", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC));
    EXPECT_EQ("gpu", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_GPU));
    EXPECT_EQ("package_integrated_gpu", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU));
    EXPECT_EQ("gpu_chip", PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_GPU_CHIP));
}

TEST_F(PlatformTopoTest, domain_name_to_type)
{
    EXPECT_THROW(PlatformTopo::domain_name_to_type("unknown"), Exception);

    EXPECT_EQ(GEOPM_DOMAIN_BOARD, PlatformTopo::domain_name_to_type("board"));
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE, PlatformTopo::domain_name_to_type("package"));
    EXPECT_EQ(GEOPM_DOMAIN_CORE, PlatformTopo::domain_name_to_type("core"));
    EXPECT_EQ(GEOPM_DOMAIN_CPU, PlatformTopo::domain_name_to_type("cpu"));
    EXPECT_EQ(GEOPM_DOMAIN_MEMORY, PlatformTopo::domain_name_to_type("memory"));
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY, PlatformTopo::domain_name_to_type("package_integrated_memory"));
    EXPECT_EQ(GEOPM_DOMAIN_NIC, PlatformTopo::domain_name_to_type("nic"));
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC, PlatformTopo::domain_name_to_type("package_integrated_nic"));
    EXPECT_EQ(GEOPM_DOMAIN_GPU, PlatformTopo::domain_name_to_type("gpu"));
    EXPECT_EQ(GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU, PlatformTopo::domain_name_to_type("package_integrated_gpu"));
    EXPECT_EQ(GEOPM_DOMAIN_GPU_CHIP, PlatformTopo::domain_name_to_type("gpu_chip"));
}

TEST_F(PlatformTopoTest, create_cache)
{
    // Delete existing cache
    const std::string cache_file_path = "PlatformTopoTest-geopm-topo-cache";
    unlink(cache_file_path.c_str());

    std::shared_ptr<MockGPUTopo> gpu_topo;
    gpu_topo = std::make_shared<MockGPUTopo>();
    EXPECT_CALL(*gpu_topo, num_gpu())
        .WillOnce(Return(6));
    EXPECT_CALL(*gpu_topo, num_gpu(GEOPM_DOMAIN_GPU))
        .WillOnce(Return(6));
    EXPECT_CALL(*gpu_topo, num_gpu(GEOPM_DOMAIN_GPU_CHIP))
        .WillOnce(Return(12));
    EXPECT_CALL(*gpu_topo, cpu_affinity_ideal(_,_))
        .WillOnce(Return(std::set<int>{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,204}))
        .WillOnce(Return(std::set<int>{34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,205}))
        .WillOnce(Return(std::set<int>{68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,206}))
        .WillOnce(Return(std::set<int>{102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,207}))
        .WillOnce(Return(std::set<int>{136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169}))
        .WillOnce(Return(std::set<int>{170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203}))
        .WillOnce(Return(std::set<int>{0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,204}))
        .WillOnce(Return(std::set<int>{1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33}))
        .WillOnce(Return(std::set<int>{34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,205}))
        .WillOnce(Return(std::set<int>{35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,65,67}))
        .WillOnce(Return(std::set<int>{68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,100,206}))
        .WillOnce(Return(std::set<int>{69,71,73,75,77,79,81,83,85,87,89,91,93,95,97,99,101}))
        .WillOnce(Return(std::set<int>{102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,207}))
        .WillOnce(Return(std::set<int>{103,105,107,109,111,113,115,117,119,121,123,125,127,129,131,133,135}))
        .WillOnce(Return(std::set<int>{136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168}))
        .WillOnce(Return(std::set<int>{137,139,141,143,145,147,149,151,153,155,157,159,161,163,165,167,169}))
        .WillOnce(Return(std::set<int>{170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,200,202}))
        .WillOnce(Return(std::set<int>{171,173,175,177,179,181,183,185,187,189,191,193,195,197,199,201,203}));
    spoof_lscpu();

    // Test case: no lscpu error, file does not exist
    setenv("PLATFORM_TOPO_TEST_LSCPU_ERROR", "", 1);

    PlatformTopoImp::create_cache(cache_file_path, *gpu_topo);

    std::ifstream cache_stream(cache_file_path);
    std::string cache_line;
    getline(cache_stream, cache_line);
    ASSERT_TRUE(geopm::string_begins_with(cache_line, "Architecture:"));
    cache_stream.close();

    // Test case: file exist, lscpu should not be called, but if it
    // does it will error.
    setenv("PLATFORM_TOPO_TEST_LSCPU_ERROR", "1", 1);
    PlatformTopoImp::create_cache(cache_file_path);

    cache_stream.open(cache_file_path);
    getline(cache_stream, cache_line);
    ASSERT_TRUE(geopm::string_begins_with(cache_line, "Architecture:"));
    cache_stream.close();

    // Test case: file does not exist and lscpu returns an error code.
    unlink(cache_file_path.c_str());
    EXPECT_THROW(PlatformTopoImp::create_cache(cache_file_path), geopm::Exception);
    struct stat stat_struct;
    ASSERT_EQ(-1, stat(cache_file_path.c_str(), &stat_struct));
}


TEST_F(PlatformTopoTest, call_c_wrappers)
{
    spoof_lscpu();
    // negative test num_domain()
    ASSERT_GT(0, geopm_topo_num_domain(GEOPM_NUM_DOMAIN));
    // simple test for num_domain()
    ASSERT_EQ(1, geopm_topo_num_domain(GEOPM_DOMAIN_BOARD));
    // negative test for domain_idx()
    ASSERT_GT(0, geopm_topo_domain_idx(GEOPM_DOMAIN_BOARD, -1));
    // simple test for domain_idx()
    ASSERT_EQ(0, geopm_topo_domain_idx(GEOPM_DOMAIN_BOARD, 0));
    // check that the CPUs are indexed properly
    int num_cpu = geopm_topo_num_domain(GEOPM_DOMAIN_CPU);
    ASSERT_LE(1, num_cpu);
    EXPECT_EQ(0, geopm_topo_domain_idx(GEOPM_DOMAIN_BOARD, num_cpu - 1));
    // another negative test for domain_idx
    EXPECT_GT(0, geopm_topo_domain_idx(GEOPM_DOMAIN_BOARD, num_cpu));
    // simple test for num_domain_nested
    ASSERT_GT(0, geopm_topo_num_domain_nested(GEOPM_DOMAIN_BOARD, GEOPM_DOMAIN_CPU));
    // simple test for num_domain_nested
    ASSERT_EQ(num_cpu, geopm_topo_num_domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_BOARD));
    // negative test for domain_nested()
    EXPECT_GT(0, geopm_topo_domain_nested(GEOPM_DOMAIN_BOARD, GEOPM_DOMAIN_CPU, 0, num_cpu, NULL));
    // simple test for domain_nested()
    std::vector<int> expect_cpu(num_cpu);
    std::vector<int> actual_cpu(num_cpu, -1);
    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        expect_cpu[cpu_idx] = cpu_idx;
    }
    EXPECT_EQ(0, geopm_topo_domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_BOARD, 0, num_cpu, actual_cpu.data()));
    EXPECT_EQ(expect_cpu, actual_cpu);
    char domain_name[NAME_MAX];
    std::string domain_name_str;
    // negative test for domain_name()
    EXPECT_GT(0, geopm_topo_domain_name(GEOPM_NUM_DOMAIN, NAME_MAX, domain_name));
    // simple test for domain_name()
    EXPECT_EQ(0, geopm_topo_domain_name(GEOPM_DOMAIN_CPU, NAME_MAX, domain_name));
    domain_name_str = domain_name;
    EXPECT_EQ("cpu", domain_name_str);
    // negative test for domain_type()
    EXPECT_GT(0, geopm_topo_domain_type("raspberry"));
    // simple test for domain_type()
    EXPECT_EQ(GEOPM_DOMAIN_CPU, geopm_topo_domain_type("cpu"));
}

TEST_F(PlatformTopoTest, check_file_too_old)
{
    spoof_lscpu();
    write_lscpu(m_hsw_lscpu_str);

    struct sysinfo si;
    sysinfo(&si);
    struct geopm_time_s current_time;
    geopm_time_real(&current_time);
    unsigned int last_boot_time = current_time.t.tv_sec - si.uptime;

    // Modify the last modified time to be prior to the last boot
    unsigned int old_time = last_boot_time - 600; // 10 minutes before boot
    struct utimbuf file_times = {old_time, old_time};
    utime(m_lscpu_file_name.c_str(), &file_times);

    // Verify the modification worked
    struct stat file_stat;
    stat(m_lscpu_file_name.c_str(), &file_stat);
    ASSERT_EQ(old_time, file_stat.st_mtime);

    PlatformTopoImp topo(m_lscpu_file_name);

    // Verify the cache was regenerated because it was too old
    stat(m_lscpu_file_name.c_str(), &file_stat);
    ASSERT_LT(last_boot_time, file_stat.st_mtime);

    // Verify the new file contents
    std::string new_file_contents = geopm::read_file(m_lscpu_file_name);
    ASSERT_EQ(m_hsw_lscpu_str, new_file_contents);
}

TEST_F(PlatformTopoTest, check_file_bad_perms)
{
    spoof_lscpu();
    write_lscpu(m_hsw_lscpu_str);

    // Override the permissions to a known bad state: 0o644
    mode_t bad_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    chmod(m_lscpu_file_name.c_str(), bad_perms);

    // Verify initial state
    struct stat file_stat;
    stat(m_lscpu_file_name.c_str(), &file_stat);
    mode_t actual_perms = file_stat.st_mode & ~S_IFMT;
    ASSERT_EQ(bad_perms, actual_perms);

    PlatformTopoImp topo(m_lscpu_file_name);

    // Verify that the cache was regenerated because it had the wrong permissions
    stat(m_lscpu_file_name.c_str(), &file_stat);
    mode_t expected_perms = S_IRUSR | S_IWUSR; // 0o600 by default
    actual_perms = file_stat.st_mode & ~S_IFMT;
    ASSERT_EQ(expected_perms, actual_perms);

    // Verify the new file contents
    std::string new_file_contents = geopm::read_file(m_lscpu_file_name);
    ASSERT_EQ(m_hsw_lscpu_str, new_file_contents);
}

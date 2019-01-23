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

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <cmath>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_hash.h"

#include "Exception.hpp"
#include "PluginFactory.hpp"
#include "CpuinfoIOGroup.hpp"
#include "PlatformTopo.hpp"
#include "geopm_test.hpp"

using geopm::IPlatformTopo;
using geopm::CpuinfoIOGroup;

class CpuinfoIOGroupTest: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        const std::string m_cpuinfo_path =     "CpuinfoIOGroupTest_cpu_info";
        const std::string m_cpufreq_min_path = "CpuinfoIOGroupTest_cpu_freq_min";
        const std::string m_cpufreq_max_path = "CpuinfoIOGroupTest_cpu_freq_max";
};

void CpuinfoIOGroupTest::SetUp()
{
    std::ofstream cpufreq_min_stream(m_cpufreq_min_path);
    cpufreq_min_stream << "1000000";
    cpufreq_min_stream.close();
    std::ofstream cpufreq_max_stream(m_cpufreq_max_path);
    cpufreq_max_stream << "2000000";
    cpufreq_max_stream.close();

}

void CpuinfoIOGroupTest::TearDown()
{
    std::remove(m_cpufreq_min_path.c_str());
    std::remove(m_cpufreq_max_path.c_str());
    std::remove(m_cpuinfo_path.c_str());
}

TEST_F(CpuinfoIOGroupTest, valid_signals)
{
    const std::string cpuinfo_str =
        "processor       : 254\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 0000 @ 1.30GHz\n"
        "stepping        : 1\n";
    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    CpuinfoIOGroup freq_limits(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path);

    // all provided signals are valid
    EXPECT_NE(0u, freq_limits.signal_names().size());
    for (const auto &sig : freq_limits.signal_names()) {
        EXPECT_TRUE(freq_limits.is_valid_signal(sig));
    }
    EXPECT_EQ(0u, freq_limits.control_names().size());
}

TEST_F(CpuinfoIOGroupTest, parse_sticker_with_at)
{
    const std::string cpuinfo_str =
        "processor       : 254\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 0000 @ 1.30GHz\n"
        "stepping        : 1\n"
        "microcode       : 0x1ac\n"
        "cpu MHz         : 1036.394\n"
        "cache size      : 1024 KB\n"
        "physical id     : 0\n"
        "siblings        : 256\n"
        "core id         : 72\n"
        "cpu cores       : 64\n"
        "apicid          : 291\n"
        "initial apicid  : 291\n"
        "fpu             : yes\n"
        "fpu_exception   : yes\n"
        "cpuid level     : 13\n"
        "wp              : yes\n"
        "flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl est tm2 ssse3 fma cx16 xtpr pdcm sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch ida arat epb pln pts dtherm fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms avx512f rdseed adx avx512pf avx512er avx512cd xsaveopt\n"
        "bogomips        : 2594.01\n"
        "clflush size    : 64\n"
        "cache_alignment : 64\n"
        "address sizes   : 46 bits physical, 48 bits virtual\n"
        "power management:\n\n";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    CpuinfoIOGroup freq_limits(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path);
    double freq = freq_limits.read_signal("CPUINFO::FREQ_STICKER", IPlatformTopo::M_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(1.3e9, freq);
}

TEST_F(CpuinfoIOGroupTest, parse_sticker_without_at)
{
    const std::string cpuinfo_str =
        "processor       : 255\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 0000 1.20GHz\n"
        "stepping        : 1\n"
        "microcode       : 0x1ac\n"
        "cpu MHz         : 1069.199\n"
        "cache size      : 1024 KB\n"
        "physical id     : 0\n"
        "siblings        : 256\n"
        "core id         : 73\n"
        "cpu cores       : 64\n"
        "apicid          : 295\n"
        "initial apicid  : 295\n"
        "fpu             : yes\n"
        "fpu_exception   : yes\n"
        "cpuid level     : 13\n"
        "wp              : yes\n"
        "flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl est tm2 ssse3 fma cx16 xtpr pdcm sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch ida arat epb pln pts dtherm fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms avx512f rdseed adx avx512pf avx512er avx512cd xsaveopt\n"
        "bogomips        : 2594.01\n"
        "clflush size    : 64\n"
        "cache_alignment : 64\n"
        "address sizes   : 46 bits physical, 48 bits virtual\n"
        "power management:\n\n";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    CpuinfoIOGroup freq_limits(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path);
    double freq = freq_limits.read_signal("CPUINFO::FREQ_STICKER", IPlatformTopo::M_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(1.2e9, freq);
}

TEST_F(CpuinfoIOGroupTest, parse_sticker_with_ghz_space)
{
    const std::string cpuinfo_str =
        "processor       : 255\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 0000 1.10 GHz\n"
        "stepping        : 1\n"
        "microcode       : 0x1ac\n"
        "cpu MHz         : 1069.199\n"
        "cache size      : 1024 KB\n"
        "physical id     : 0\n"
        "siblings        : 256\n"
        "core id         : 73\n"
        "cpu cores       : 64\n"
        "apicid          : 295\n"
        "initial apicid  : 295\n"
        "fpu             : yes\n"
        "fpu_exception   : yes\n"
        "cpuid level     : 13\n"
        "wp              : yes\n"
        "flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl est tm2 ssse3 fma cx16 xtpr pdcm sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch ida arat epb pln pts dtherm fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms avx512f rdseed adx avx512pf avx512er avx512cd xsaveopt\n"
        "bogomips        : 2594.01\n"
        "clflush size    : 64\n"
        "cache_alignment : 64\n"
        "address sizes   : 46 bits physical, 48 bits virtual\n"
        "power management:\n\n";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    CpuinfoIOGroup freq_limits(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path);
    double freq = freq_limits.read_signal("CPUINFO::FREQ_STICKER", IPlatformTopo::M_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(1.1e9, freq);
}

TEST_F(CpuinfoIOGroupTest, parse_sticker_missing_newline)
{
    const std::string cpuinfo_str =
        "processor       : 255\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 0000 1.10GHz";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    CpuinfoIOGroup freq_limits(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path);
    double freq = freq_limits.read_signal("CPUINFO::FREQ_STICKER", IPlatformTopo::M_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(1.1e9, freq);
}

TEST_F(CpuinfoIOGroupTest, parse_error_no_sticker)
{
    const std::string cpuinfo_str =
        "processor       : 255\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name      : Intel(R) Genuine Intel(R) CPU GHz\n"
        "stepping        : 1";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    GEOPM_EXPECT_THROW_MESSAGE(
        CpuinfoIOGroup(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path),
        GEOPM_ERROR_INVALID, "Invalid frequency");
}

TEST_F(CpuinfoIOGroupTest, parse_sticker_multiple_ghz)
{
    std::string cpuinfo_str =
        "processor       : 255\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 8.7GHz\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 1.5GHz\n"
        "stepping        : 1.0GHz\n";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    CpuinfoIOGroup freq_limits(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path);
    double freq = freq_limits.read_signal("CPUINFO::FREQ_STICKER", IPlatformTopo::M_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(1.5e9, freq);
}

TEST_F(CpuinfoIOGroupTest, parse_sticker_multiple_model_name)
{
    const std::string cpuinfo_str =
        "processor       : 254\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name X    : Intel(R) Genuine Intel(R) CPU 0000 @ 1.00GHz\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 0000 @ 1.30GHz\n"
        "stepping        : 1\n"
        "microcode       : 0x1ac\n"
        "cpu MHz         : 1036.394\n"
        "cache size      : 1024 KB\n"
        "physical id     : 0\n"
        "siblings        : 256\n"
        "core id         : 72\n"
        "cpu cores       : 64\n"
        "apicid          : 291\n"
        "initial apicid  : 291\n"
        "fpu             : yes\n"
        "fpu_exception   : yes\n"
        "cpuid level     : 13\n"
        "wp              : yes\n"
        "flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl est tm2 ssse3 fma cx16 xtpr pdcm sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch ida arat epb pln pts dtherm fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms avx512f rdseed adx avx512pf avx512er avx512cd xsaveopt\n"
        "bogomips        : 2594.01\n"
        "clflush size    : 64\n"
        "cache_alignment : 64\n"
        "address sizes   : 46 bits physical, 48 bits virtual\n"
        "power management:\n\n";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    CpuinfoIOGroup freq_limits(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path);
    double freq = freq_limits.read_signal("CPUINFO::FREQ_STICKER", IPlatformTopo::M_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(1.3e9, freq);
}

TEST_F(CpuinfoIOGroupTest, parse_cpu_freq)
{
    const std::string cpuinfo_str =
        "processor       : 254\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name X    : Intel(R) Genuine Intel(R) CPU 0000 @ 1.00GHz\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 0000 @ 1.30GHz\n"
        "stepping        : 1\n"
        "microcode       : 0x1ac\n"
        "cpu MHz         : 1036.394\n"
        "cache size      : 1024 KB\n"
        "physical id     : 0\n"
        "siblings        : 256\n"
        "core id         : 72\n"
        "cpu cores       : 64\n"
        "apicid          : 291\n"
        "initial apicid  : 291\n"
        "fpu             : yes\n"
        "fpu_exception   : yes\n"
        "cpuid level     : 13\n"
        "wp              : yes\n"
        "flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl est tm2 ssse3 fma cx16 xtpr pdcm sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch ida arat epb pln pts dtherm fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms avx512f rdseed adx avx512pf avx512er avx512cd xsaveopt\n"
        "bogomips        : 2594.01\n"
        "clflush size    : 64\n"
        "cache_alignment : 64\n"
        "address sizes   : 46 bits physical, 48 bits virtual\n"
        "power management:\n\n";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    CpuinfoIOGroup freq_limits(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path);
    double freq = freq_limits.read_signal("CPUINFO::FREQ_MIN", IPlatformTopo::M_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(1.0e9, freq);
    freq = freq_limits.read_signal("CPUINFO::FREQ_MAX", IPlatformTopo::M_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(2.0e9, freq);
}

TEST_F(CpuinfoIOGroupTest, plugin)
{
    const std::string cpuinfo_str =
        "processor       : 254\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name X    : Intel(R) Genuine Intel(R) CPU 0000 @ 1.00GHz\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 0000 @ 1.30GHz\n"
        "stepping        : 1\n"
        "microcode       : 0x1ac\n"
        "cpu MHz         : 1036.394\n"
        "cache size      : 1024 KB\n"
        "physical id     : 0\n"
        "siblings        : 256\n"
        "core id         : 72\n"
        "cpu cores       : 64\n"
        "apicid          : 291\n"
        "initial apicid  : 291\n"
        "fpu             : yes\n"
        "fpu_exception   : yes\n"
        "cpuid level     : 13\n"
        "wp              : yes\n"
        "flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl est tm2 ssse3 fma cx16 xtpr pdcm sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch ida arat epb pln pts dtherm fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms avx512f rdseed adx avx512pf avx512er avx512cd xsaveopt\n"
        "bogomips        : 2594.01\n"
        "clflush size    : 64\n"
        "cache_alignment : 64\n"
        "address sizes   : 46 bits physical, 48 bits virtual\n"
        "power management:\n\n";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    EXPECT_EQ("CPUINFO", CpuinfoIOGroup(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path).plugin_name());
}


TEST_F(CpuinfoIOGroupTest, parse_error_sticker_bad_path)
{
    const std::string cpuinfo_str =
        "processor       : 255\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 0000 1.10GHz";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();

    GEOPM_EXPECT_THROW_MESSAGE(
        CpuinfoIOGroup("/bad/path", m_cpufreq_min_path, m_cpufreq_max_path),
        GEOPM_ERROR_RUNTIME, "Failed to open");

    GEOPM_EXPECT_THROW_MESSAGE(
        CpuinfoIOGroup(m_cpuinfo_path, "/bad/path", m_cpufreq_max_path),
        GEOPM_ERROR_RUNTIME, "Failed to open");

    GEOPM_EXPECT_THROW_MESSAGE(
        CpuinfoIOGroup(m_cpuinfo_path, m_cpufreq_min_path, "/bad/path"),
        GEOPM_ERROR_RUNTIME, "Failed to open");
}

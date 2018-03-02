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

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_hash.h"

#include "Exception.hpp"
#include "Decider.hpp"
#include "EfficientFreqDecider.hpp"
#include "Decider.hpp"
#include "PlatformTopo.hpp"

#include "MockRegion.hpp"
#include "MockPolicy.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "geopm.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Sequence;
using ::testing::Return;
using geopm::IDecider;
using geopm::EfficientFreqDecider;

class EfficientFreqDeciderTest: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        static const size_t M_NUM_REGIONS = 5;
        std::vector<size_t> m_hints;
        std::vector<double> m_expected_freqs;
        std::unique_ptr<MockRegion> m_mock_region;
        std::unique_ptr<MockPolicy> m_mock_policy;
        std::unique_ptr<IDecider> m_decider;
        std::vector<std::string> m_region_names;
        std::vector<double> m_mapped_freqs;
        double m_freq_min;
        double m_freq_max;
        MockPlatformIO *m_platform_io;
        MockPlatformTopo *m_platform_topo;
        const std::string m_cpuinfo_path = "EfficientFreqDeciderTest_cpu_info";
        const std::string m_cpufreq_min_path = "EfficientFreqDeciderTest_cpu_freq_min";
        const std::string m_cpufreq_max_path = "EfficientFreqDeciderTest_cpu_freq_max";
};

void EfficientFreqDeciderTest::SetUp()
{
    m_platform_io = new MockPlatformIO;
    m_platform_topo = new MockPlatformTopo;
    ON_CALL(*m_platform_io, control_domain_type(_))
            .WillByDefault(Return(geopm::PlatformTopo::M_DOMAIN_CPU));
    ON_CALL(*m_platform_topo, num_domain(geopm::PlatformTopo::M_DOMAIN_CPU))
            .WillByDefault(Return(1));

    setenv("GEOPM_PLUGIN_PATH", ".libs/", 1);

    m_freq_min = 1800000000.0;
    m_freq_max = 2200000000.0;
    m_region_names = {"mapped_region0", "mapped_region1", "mapped_region2", "mapped_region3", "mapped_region4"};
    m_mapped_freqs = {m_freq_max, 2100000000.0, 2000000000.0, 1900000000.0, m_freq_min};

    m_hints = {GEOPM_REGION_HINT_UNKNOWN, GEOPM_REGION_HINT_COMPUTE, GEOPM_REGION_HINT_MEMORY,
               GEOPM_REGION_HINT_NETWORK, GEOPM_REGION_HINT_IO, GEOPM_REGION_HINT_SERIAL,
               GEOPM_REGION_HINT_PARALLEL, GEOPM_REGION_HINT_IGNORE};
    m_expected_freqs = {m_freq_min, m_freq_max, m_freq_min, m_freq_max, m_freq_min};

    EXPECT_EQ(m_mapped_freqs.size(), m_region_names.size());

    std::stringstream ss;
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        ss << m_region_names[x] << ":" << m_mapped_freqs[x] << ",";
    }

    setenv("GEOPM_EFFICIENT_FREQ_MIN", std::to_string(m_freq_min).c_str(), 1);
    setenv("GEOPM_EFFICIENT_FREQ_MAX", std::to_string(m_freq_max).c_str(), 1);
    setenv("GEOPM_EFFICIENT_FREQ_RID_MAP", ss.str().c_str(), 1);

    m_mock_region = std::unique_ptr<MockRegion>(new MockRegion());
    m_mock_policy = std::unique_ptr<MockPolicy>(new MockPolicy());
    m_decider = std::unique_ptr<IDecider>(new EfficientFreqDecider(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path, *m_platform_io, *m_platform_topo));
}

void EfficientFreqDeciderTest::TearDown()
{
    unsetenv("GEOPM_EFFICIENT_FREQ_ONLINE");
    unsetenv("GEOPM_EFFICIENT_FREQ_MIN");
    unsetenv("GEOPM_EFFICIENT_FREQ_MAX");
    std::remove(m_cpufreq_min_path.c_str());
    std::remove(m_cpufreq_max_path.c_str());
    std::remove(m_cpuinfo_path.c_str());
    delete m_platform_topo;
    delete m_platform_io;
}

TEST_F(EfficientFreqDeciderTest, parse_cpu_info0)
{
    unsetenv("GEOPM_EFFICIENT_FREQ_MIN");
    unsetenv("GEOPM_EFFICIENT_FREQ_MAX");
    unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");

    // Test cases where we need CPU info (no cpufreq driver)
    std::remove(m_cpufreq_min_path.c_str());
    std::remove(m_cpufreq_max_path.c_str());

    // with @
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
    EfficientFreqDecider decider(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path, *m_platform_io, *m_platform_topo);
    double freq = decider.cpu_freq_sticker();
    EXPECT_DOUBLE_EQ(1.3e9, freq);
    std::remove(m_cpuinfo_path.c_str());
}

TEST_F(EfficientFreqDeciderTest, parse_cpu_info1)
{
    unsetenv("GEOPM_EFFICIENT_FREQ_MIN");
    unsetenv("GEOPM_EFFICIENT_FREQ_MAX");
    unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");

    // Test cases where we need CPU info (no cpufreq driver)
    std::remove(m_cpufreq_min_path.c_str());
    std::remove(m_cpufreq_max_path.c_str());

    // without @
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
    EfficientFreqDecider decider(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path, *m_platform_io, *m_platform_topo);
    double freq = decider.cpu_freq_sticker();
    EXPECT_DOUBLE_EQ(1.2e9, freq);
    std::remove(m_cpuinfo_path.c_str());
}

TEST_F(EfficientFreqDeciderTest, parse_cpu_info2)
{
    unsetenv("GEOPM_EFFICIENT_FREQ_MIN");
    unsetenv("GEOPM_EFFICIENT_FREQ_MAX");
    unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");

    // Test cases where we need CPU info (no cpufreq driver)
    std::remove(m_cpufreq_min_path.c_str());
    std::remove(m_cpufreq_max_path.c_str());

    // without @ with space
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
    EfficientFreqDecider decider(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path, *m_platform_io, *m_platform_topo);
    double freq = decider.cpu_freq_sticker();
    EXPECT_DOUBLE_EQ(1.1e9, freq);
    std::remove(m_cpuinfo_path.c_str());
}

TEST_F(EfficientFreqDeciderTest, parse_cpu_info3)
{
    unsetenv("GEOPM_EFFICIENT_FREQ_MIN");
    unsetenv("GEOPM_EFFICIENT_FREQ_MAX");
    unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");

    // Test cases where we need CPU info (no cpufreq driver)
    std::remove(m_cpufreq_min_path.c_str());
    std::remove(m_cpufreq_max_path.c_str());

    // missing newline
    const std::string cpuinfo_str =
        "processor       : 255\n"
        "vendor_id       : GenuineIntel\n"
        "cpu family      : 6\n"
        "model           : 87\n"
        "model name      : Intel(R) Genuine Intel(R) CPU 0000 1.10GHz";

    std::ofstream cpuinfo_stream(m_cpuinfo_path);
    cpuinfo_stream << cpuinfo_str;
    cpuinfo_stream.close();
    EfficientFreqDecider decider(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path, *m_platform_io, *m_platform_topo);
    double freq = decider.cpu_freq_sticker();
    EXPECT_DOUBLE_EQ(1.1e9, freq);
    std::remove(m_cpuinfo_path.c_str());
}

TEST_F(EfficientFreqDeciderTest, parse_cpu_info4)
{
    unsetenv("GEOPM_EFFICIENT_FREQ_MIN");
    unsetenv("GEOPM_EFFICIENT_FREQ_MAX");
    unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");

    // Test cases where we need CPU info (no cpufreq driver)
    std::remove(m_cpufreq_min_path.c_str());
    std::remove(m_cpufreq_max_path.c_str());

    // missing number
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
    EXPECT_THROW( {
            EfficientFreqDecider decider(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path, *m_platform_io, *m_platform_topo);
        },
        geopm::Exception);
    std::remove(m_cpuinfo_path.c_str());
}

TEST_F(EfficientFreqDeciderTest, parse_cpu_info5)
{
    unsetenv("GEOPM_EFFICIENT_FREQ_MIN");
    unsetenv("GEOPM_EFFICIENT_FREQ_MAX");
    unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");

    // Test cases where we need CPU info (no cpufreq driver)
    std::remove(m_cpufreq_min_path.c_str());
    std::remove(m_cpufreq_max_path.c_str());

    // multiple GHz
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
    EfficientFreqDecider decider(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path, *m_platform_io, *m_platform_topo);
    double freq = decider.cpu_freq_sticker();
    EXPECT_DOUBLE_EQ(1.5e9, freq);
    std::remove(m_cpuinfo_path.c_str());
}

TEST_F(EfficientFreqDeciderTest, parse_cpu_info6)
{
    unsetenv("GEOPM_EFFICIENT_FREQ_MIN");
    unsetenv("GEOPM_EFFICIENT_FREQ_MAX");
    unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");

    // Test cases where we need CPU info (no cpufreq driver)
    std::remove(m_cpufreq_min_path.c_str());
    std::remove(m_cpufreq_max_path.c_str());

    // with model name foobar
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
    EfficientFreqDecider decider(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path, *m_platform_io, *m_platform_topo);
    double freq = decider.cpu_freq_sticker();
    EXPECT_DOUBLE_EQ(1.3e9, freq);
    std::remove(m_cpuinfo_path.c_str());
}

TEST_F(EfficientFreqDeciderTest, parse_cpu_freq)
{
    unsetenv("GEOPM_EFFICIENT_FREQ_MIN");
    unsetenv("GEOPM_EFFICIENT_FREQ_MAX");
    unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");

    // Test cases where we need CPU info (no cpufreq driver)
    std::ofstream cpufreq_min_stream(m_cpufreq_min_path);
    cpufreq_min_stream << "1000000";
    cpufreq_min_stream.close();
    std::ofstream cpufreq_max_stream(m_cpufreq_max_path);
    cpufreq_max_stream << "2000000";
    cpufreq_max_stream.close();

    EfficientFreqDecider decider(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path, *m_platform_io, *m_platform_topo);
    double freq = decider.cpu_freq_min();
    EXPECT_DOUBLE_EQ(1.0e9, freq);
    freq = decider.cpu_freq_max();
    EXPECT_DOUBLE_EQ(2.0e9, freq);

    std::remove(m_cpufreq_min_path.c_str());
    std::remove(m_cpufreq_max_path.c_str());
}

TEST_F(EfficientFreqDeciderTest, map)
{
    Sequence mock_seq;
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        EXPECT_CALL(*m_mock_region, identifier())
            .InSequence(mock_seq)
            // one for super, once for our decider
            .WillOnce(Return(geopm_crc32_str(0, m_region_names[x].c_str())))
            .WillOnce(Return(geopm_crc32_str(0, m_region_names[x].c_str())));
    }

    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        m_decider->update_policy(*m_mock_region, *m_mock_policy);
    }
}

TEST_F(EfficientFreqDeciderTest, plugin)
{
    EXPECT_EQ("efficient_freq", geopm::decider_factory().make_plugin("efficient_freq")->name());
}


TEST_F(EfficientFreqDeciderTest, decider_is_supported)
{
    EXPECT_TRUE(m_decider->decider_supported("efficient_freq"));
    EXPECT_FALSE(m_decider->decider_supported("bad_string"));
}

TEST_F(EfficientFreqDeciderTest, name)
{
    EXPECT_EQ("efficient_freq", m_decider->name());
}

TEST_F(EfficientFreqDeciderTest, hint)
{
    Sequence mock_seq;
    for (size_t x = 0; x < m_hints.size(); x++) {
        EXPECT_CALL(*m_mock_region, hint())
            .InSequence(mock_seq)
            .WillOnce(testing::Return(m_hints[x]));
    }

    for (size_t x = 0; x < m_hints.size(); x++) {
        m_decider->update_policy(*m_mock_region, *m_mock_policy);
    }
}

TEST_F(EfficientFreqDeciderTest, online_mode)
{
    int err = unsetenv("GEOPM_EFFICIENT_FREQ_RID_MAP");
    EXPECT_EQ(0, err);
    EXPECT_EQ(NULL, getenv("GEOPM_EFFICIENT_FREQ_RID_MAP"));
    setenv("GEOPM_EFFICIENT_FREQ_ONLINE", "yes", 1);
    setenv("GEOPM_EFFICIENT_FREQ_MIN", "1e9", 1);
    setenv("GEOPM_EFFICIENT_FREQ_MAX", "2e9", 1);

    // reset decider with new settings
    m_decider = std::unique_ptr<IDecider>(new EfficientFreqDecider(m_cpuinfo_path, m_cpufreq_min_path, m_cpufreq_max_path, *m_platform_io, *m_platform_topo));

    {
        // should not be called if we hit the adaptive branch
        EXPECT_CALL(*m_mock_region, hint()).Times(0);

        EXPECT_CALL(*m_mock_region, num_sample(_, _));
        EXPECT_CALL(*m_mock_region, identifier()).Times(2);

        m_decider->update_policy(*m_mock_region, *m_mock_policy);
    }

    {
        EXPECT_CALL(*m_mock_region, hint()).Times(0);
        EXPECT_CALL(*m_mock_region, num_sample(_, _));

        // upon second update, previous region will not be null
        // and it will check the region id
        EXPECT_CALL(*m_mock_region, identifier()).Times(3);

        m_decider->update_policy(*m_mock_region, *m_mock_policy);
    }

    {
        EXPECT_CALL(*m_mock_region, hint()).Times(0);
        EXPECT_CALL(*m_mock_region, num_sample(_, _));

        // cause a transition to a new region
        EXPECT_CALL(*m_mock_region, identifier()).Times(4)
            .WillOnce(Return(1))
            .WillOnce(Return(2))
            .WillOnce(Return(3))
            .WillOnce(Return(4));

        // get runtime
        EXPECT_CALL(*m_mock_region, signal(_, _))
            .WillOnce(Return(0));

        m_decider->update_policy(*m_mock_region, *m_mock_policy);
    }
}

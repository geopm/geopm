/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <cmath>

extern "C"
{
    static int g_cpuid_sticker = 0;
    static int mock_cpuid(unsigned int leaf, unsigned int *eax, unsigned int *ebx,
                          unsigned int *ecx, unsigned int *edx)
    {
        *eax = g_cpuid_sticker;
        return 0;
    }
#define __get_cpuid(p0, p1, p2, p3, p4) mock_cpuid(p0, p1, p2, p3, p4)
}
#define GEOPM_TEST
#include "src/CpuinfoIOGroup.cpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_hash.h"

#include "geopm/Exception.hpp"
#include "geopm/PluginFactory.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_test.hpp"

using geopm::PlatformTopo;
using geopm::CpuinfoIOGroup;
using geopm::Exception;
using geopm::IOGroup;

class CpuinfoIOGroupTest: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        const std::string m_cpufreq_min_path = "CpuinfoIOGroupTest_cpu_freq_min";
        const std::string m_cpufreq_max_path = "CpuinfoIOGroupTest_cpu_freq_max";
};

void CpuinfoIOGroupTest::SetUp()
{
    g_cpuid_sticker = 1300;
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
}

TEST_F(CpuinfoIOGroupTest, valid_signals)
{
    CpuinfoIOGroup freq_limits(m_cpufreq_min_path, m_cpufreq_max_path);

    // all provided signals are valid
    EXPECT_NE(0u, freq_limits.signal_names().size());
    for (const auto &sig : freq_limits.signal_names()) {
        EXPECT_TRUE(freq_limits.is_valid_signal(sig));
        EXPECT_EQ(IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT, freq_limits.signal_behavior(sig));
    }
    EXPECT_EQ(0u, freq_limits.control_names().size());
}

TEST_F(CpuinfoIOGroupTest, read_signal)
{
    CpuinfoIOGroup freq_limits(m_cpufreq_min_path, m_cpufreq_max_path);
    double freq = freq_limits.read_signal("CPUINFO::FREQ_STICKER", GEOPM_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(1.3e9, freq);

    // cannot read from wrong domain
    EXPECT_THROW(freq_limits.read_signal("CPUINFO::FREQ_STICKER", GEOPM_DOMAIN_PACKAGE, 0),
                 Exception);
}

TEST_F(CpuinfoIOGroupTest, cpuid_sticker_not_supported)
{
    g_cpuid_sticker = 0;
    GEOPM_EXPECT_THROW_MESSAGE(CpuinfoIOGroup (m_cpufreq_min_path, m_cpufreq_max_path),
                               GEOPM_ERROR_INVALID, "not supported");
}

TEST_F(CpuinfoIOGroupTest, push_signal)
{
    CpuinfoIOGroup freq_limits(m_cpufreq_min_path, m_cpufreq_max_path);

    int idx = freq_limits.push_signal("CPUINFO::FREQ_STICKER", GEOPM_DOMAIN_BOARD, 0);
    EXPECT_GT(idx, 0);
    freq_limits.read_batch();
    double freq = freq_limits.sample(idx);
    EXPECT_DOUBLE_EQ(1.3e9, freq);

    // cannot push to wrong domain
    EXPECT_THROW(freq_limits.push_signal("CPUINFO::FREQ_STICKER", GEOPM_DOMAIN_PACKAGE, 0),
                 Exception);
}

TEST_F(CpuinfoIOGroupTest, plugin)
{
    EXPECT_EQ("CPUINFO", CpuinfoIOGroup(m_cpufreq_min_path, m_cpufreq_max_path).plugin_name());
}



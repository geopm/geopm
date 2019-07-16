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
#include "CNLIOGroup.hpp"
#include "PlatformTopo.hpp"
#include "geopm_test.hpp"
#include <sys/stat.h>

using geopm::PlatformTopo;
using geopm::CNLIOGroup;
using geopm::Exception;

class CNLIOGroupTest: public :: testing :: Test
{
    protected:
        void SetUp() override;
        void TearDown() override;
        const std::string m_test_dir = "CNLIOGroupTest_counters";
        const std::string m_power_path = m_test_dir + "/power";
        const std::string m_energy_path = m_test_dir + "/energy";
        const std::string m_memory_power_path = m_test_dir + "/memory_power";
        const std::string m_memory_energy_path = m_test_dir + "/memory_energy";
        const std::string m_cpu_power_path = m_test_dir + "/cpu_power";
        const std::string m_cpu_energy_path = m_test_dir + "/cpu_energy";
};

void CNLIOGroupTest::SetUp()
{
    mkdir(m_test_dir.c_str(), S_IRWXU);
    std::ofstream(m_power_path) << "85 W\n";
    std::ofstream(m_energy_path) << "598732067 J\n";
    std::ofstream(m_memory_power_path) << "6 W\n";
    std::ofstream(m_memory_energy_path) << "58869289 J\n";
    std::ofstream(m_cpu_power_path) << "33 W\n";
    std::ofstream(m_cpu_energy_path) << "374953759 J\n";
}

void CNLIOGroupTest::TearDown()
{

    std::remove(m_power_path.c_str());
    std::remove(m_energy_path.c_str());
    std::remove(m_memory_power_path.c_str());
    std::remove(m_memory_energy_path.c_str());
    std::remove(m_cpu_power_path.c_str());
    std::remove(m_cpu_energy_path.c_str());
    std::remove(m_test_dir.c_str());
}

TEST_F(CNLIOGroupTest, valid_signals)
{
    CNLIOGroup cnl(m_test_dir);

    // All provided signals are valid
    EXPECT_NE(0u, cnl.signal_names().size());
    for (const auto &sig : cnl.signal_names()) {
        EXPECT_TRUE(cnl.is_valid_signal(sig));
    }
    EXPECT_EQ(0u, cnl.control_names().size());
}

TEST_F(CNLIOGroupTest, read_signal)
{
    std::ofstream(m_power_path) << "85 W\n";
    CNLIOGroup cnl(m_test_dir);
    double power = cnl.read_signal("CNL::POWER_BOARD", GEOPM_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(85, power);

    // Cannot read from wrong domain
    EXPECT_THROW(cnl.read_signal("CNL::POWER_BOARD", GEOPM_DOMAIN_PACKAGE, 0),
                 Exception);
}

TEST_F(CNLIOGroupTest, push_signal)
{
    std::ofstream(m_power_path) << "85 W\n";
    CNLIOGroup cnl(m_test_dir);

    auto idx = cnl.push_signal("CNL::POWER_BOARD", GEOPM_DOMAIN_BOARD, 0);
    cnl.read_batch();
    double power = cnl.sample(idx);
    EXPECT_DOUBLE_EQ(85, power);

    // cannot push to wrong domain
    EXPECT_THROW(cnl.push_signal("CNL::POWER_BOARD", GEOPM_DOMAIN_PACKAGE, 0),
                 Exception);
}


TEST_F(CNLIOGroupTest, parse_power)
{
    const std::vector<std::pair<std::string, std::string>> power_signals = {
        { m_power_path, "CNL::POWER_BOARD" },
        { m_power_path, "POWER_BOARD" },
        { m_memory_power_path, "CNL::POWER_BOARD_MEMORY" },
        { m_memory_power_path, "POWER_BOARD_MEMORY" },
        { m_cpu_power_path, "CNL::POWER_BOARD_CPU" },
        { m_cpu_power_path, "POWER_BOARD_CPU" },
    };
    CNLIOGroup cnl(m_test_dir);

    // Expected format
    for (const auto &signal : power_signals) {
        std::ofstream(signal.first) << "85 W\n";
        double power = cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0);
        EXPECT_DOUBLE_EQ(85, power) << signal.second;
    }

    // Unexpected units
    for (const auto &signal : power_signals) {
        std::ofstream(signal.first) << "85 WW\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0),
                     Exception) << signal.second;
    }

    // Truncated
    for (const auto &signal : power_signals) {
        std::ofstream(signal.first) << "85";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0),
                     Exception) << signal.second;
    }
}

TEST_F(CNLIOGroupTest, parse_energy)
{
    const std::vector<std::pair<std::string, std::string>> energy_signals = {
        { m_energy_path, "CNL::ENERGY_BOARD" },
        { m_energy_path, "ENERGY_BOARD" },
        { m_memory_energy_path, "CNL::ENERGY_BOARD_MEMORY" },
        { m_memory_energy_path, "ENERGY_BOARD_MEMORY" },
        { m_cpu_energy_path, "CNL::ENERGY_BOARD_CPU" },
        { m_cpu_energy_path, "ENERGY_BOARD_CPU" },
    };
    CNLIOGroup cnl(m_test_dir);

    // Expected format
    for (const auto &signal : energy_signals) {
        std::ofstream(signal.first) << "1234567 J\n";
        double energy = cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0);
        EXPECT_DOUBLE_EQ(1234567, energy) << signal.second;
    }

    // Unexpected units
    for (const auto &signal : energy_signals) {
        std::ofstream(signal.first) << "1234567 W\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0),
                     Exception) << signal.second;
    }

    // Truncated
    for (const auto &signal : energy_signals) {
        std::ofstream(signal.first) << "1234567";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0),
                     Exception) << signal.second;
    }
}

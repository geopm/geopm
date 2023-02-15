/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CNLIOGroup.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "geopm/Exception.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PluginFactory.hpp"
#include "geopm_hash.h"
#include "geopm_test.hpp"
#include <sys/stat.h>

using geopm::CNLIOGroup;
using geopm::Exception;
using geopm::PlatformTopo;

class CNLIOGroupTest : public ::testing ::Test
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
        const std::string m_freshness_path = m_test_dir + "/freshness";
        const std::string m_raw_scan_hz_path = m_test_dir + "/raw_scan_hz";
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
    std::ofstream(m_freshness_path) << "0\n";
    std::ofstream(m_raw_scan_hz_path) << "10\n";
}

void CNLIOGroupTest::TearDown()
{

    unlink(m_power_path.c_str());
    unlink(m_energy_path.c_str());
    unlink(m_memory_power_path.c_str());
    unlink(m_memory_energy_path.c_str());
    unlink(m_cpu_power_path.c_str());
    unlink(m_cpu_energy_path.c_str());
    unlink(m_freshness_path.c_str());
    unlink(m_raw_scan_hz_path.c_str());
    rmdir(m_test_dir.c_str());
}

TEST_F(CNLIOGroupTest, valid_signals)
{
    CNLIOGroup cnl(m_test_dir);

    // All provided signals are valid
    EXPECT_NE(0u, cnl.signal_names().size());
    for (const auto &sig : cnl.signal_names()) {
        EXPECT_TRUE(cnl.is_valid_signal(sig));
        EXPECT_LT(-1, cnl.signal_behavior(sig));
    }
    EXPECT_EQ(0u, cnl.control_names().size());
}

TEST_F(CNLIOGroupTest, read_signal)
{
    std::ofstream(m_power_path) << "85 W\n";
    CNLIOGroup cnl(m_test_dir);
    double power = cnl.read_signal("CNL::BOARD_POWER", GEOPM_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(85, power);

    // Can read an updated value without recreating the IOGroup
    std::ofstream(m_power_path) << "99 W\n";
    power = cnl.read_signal("CNL::BOARD_POWER", GEOPM_DOMAIN_BOARD, 0);
    EXPECT_DOUBLE_EQ(99, power);

    // Cannot read from wrong domain
    EXPECT_THROW(cnl.read_signal("CNL::BOARD_POWER", GEOPM_DOMAIN_PACKAGE, 0), Exception);
}

TEST_F(CNLIOGroupTest, push_signal)
{
    std::ofstream(m_power_path) << "85 W\n";
    CNLIOGroup cnl(m_test_dir);

    auto idx = cnl.push_signal("CNL::BOARD_POWER", GEOPM_DOMAIN_BOARD, 0);
    cnl.read_batch();
    double power = cnl.sample(idx);
    EXPECT_DOUBLE_EQ(85, power);

    // Can read an updated value without recreating the IOGroup
    std::ofstream(m_power_path) << "100 W\n";
    cnl.read_batch();
    power = cnl.sample(idx);
    EXPECT_DOUBLE_EQ(100, power);

    // cannot push to wrong domain
    EXPECT_THROW(cnl.push_signal("CNL::BOARD_POWER", GEOPM_DOMAIN_PACKAGE, 0), Exception);
}

TEST_F(CNLIOGroupTest, parse_power)
{
    const std::vector<std::pair<std::string, std::string> > power_signals = {
        { m_power_path, "CNL::BOARD_POWER" },
        { m_power_path, "BOARD_POWER" },
        { m_memory_power_path, "CNL::MEMORY_POWER" },
        { m_cpu_power_path, "CNL::BOARD_POWER_CPU" },
    };
    CNLIOGroup cnl(m_test_dir);

    for (const auto &signal : power_signals) {
        // Expected format
        std::ofstream(signal.first) << "85 W\n";
        double power = cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0);
        EXPECT_DOUBLE_EQ(85, power) << signal.second;

        std::string sparse_string("85 W\n");
        sparse_string.resize(4096);
        std::ofstream(signal.first) << sparse_string;
        power = cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0);
        EXPECT_DOUBLE_EQ(85, power) << signal.second;

        // Unexpected units
        std::ofstream(signal.first) << "85 WW\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0), Exception)
            << signal.second;

        std::ofstream(signal.first) << "85W\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0), Exception)
            << signal.second;

        std::ofstream(signal.first) << "85";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0), Exception)
            << signal.second;

        std::ofstream(signal.first) << "85 💡\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0), Exception)
            << signal.second;

        std::ofstream(signal.first) << "Eighty-five Watts\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0),
                     std::invalid_argument)
            << signal.second;

        std::ofstream(signal.first) << "";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0), Exception)
            << signal.second;

        std::ofstream(signal.first) << "\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0),
                     std::invalid_argument)
            << signal.second;
    }
}

TEST_F(CNLIOGroupTest, parse_energy)
{
    const std::vector<std::pair<std::string, std::string> > energy_signals = {
        { m_energy_path, "CNL::BOARD_ENERGY" },
        { m_energy_path, "BOARD_ENERGY" },
        { m_memory_energy_path, "CNL::MEMORY_ENERGY" },
        { m_cpu_energy_path, "CNL::BOARD_ENERGY_CPU" },
    };
    CNLIOGroup cnl(m_test_dir);

    for (const auto &signal : energy_signals) {
        // Expected format
        std::ofstream(signal.first) << "1234567 J\n";
        double energy = cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0);
        EXPECT_DOUBLE_EQ(1234567, energy) << signal.second;

        std::string sparse_string("1234567 J\n");
        sparse_string.resize(4096);
        std::ofstream(signal.first) << sparse_string;
        energy = cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0);
        EXPECT_DOUBLE_EQ(1234567, energy) << signal.second;

        // Unexpected units
        std::ofstream(signal.first) << "1234567 W\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0), Exception)
            << signal.second;

        std::ofstream(signal.first) << "1234567J\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0), Exception)
            << signal.second;

        std::ofstream(signal.first) << "1234567";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0), Exception)
            << signal.second;

        std::ofstream(signal.first) << "1234567 ⚡\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0), Exception)
            << signal.second;

        std::ofstream(signal.first) << "Energy!\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0),
                     std::invalid_argument)
            << signal.second;

        std::ofstream(signal.first) << "";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0), Exception)
            << signal.second;

        std::ofstream(signal.first) << "\n";
        EXPECT_THROW(cnl.read_signal(signal.second, GEOPM_DOMAIN_BOARD, 0),
                     std::invalid_argument)
            << signal.second;
    }
}

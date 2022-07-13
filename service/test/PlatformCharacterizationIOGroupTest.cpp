/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include "config.h"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PluginFactory.hpp"
#include "PlatformCharacterizationIOGroup.hpp"
#include "geopm_test.hpp"
#include "MockDCGMDevicePool.hpp"
#include "MockPlatformTopo.hpp"

#include "geopm_time.h"

using geopm::PlatformCharacterizationIOGroup;
using geopm::PlatformTopo;
using geopm::Exception;
using testing::Return;

class PlatformCharacterizationIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void write_characterization(const std::string &characterization_str);

        std::unique_ptr<MockPlatformTopo> m_platform_topo;

        std::string m_characterization_file_name;
        std::string m_default_characterization_str;
};

void PlatformCharacterizationIOGroupTest::write_characterization(const std::string &characterization_str)
{
    std::ofstream characterization_fid(m_characterization_file_name);
    characterization_fid << characterization_str;
    characterization_fid.close();
    // Set perms to 0o600 to ensure test file is used and not regenerated
    mode_t default_perms = S_IRUSR | S_IWUSR;
    chmod(m_characterization_file_name.c_str(), default_perms);
}

void PlatformCharacterizationIOGroupTest::SetUp()
{
    const int num_board = 1;
    const int num_package = 2;
    const int num_gpu = 4;
    const int num_core = 20;
    const int num_cpu = 40;

    m_platform_topo = geopm::make_unique<MockPlatformTopo>();

    //Platform Topo prep
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(num_board));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(num_package));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
        .WillByDefault(Return(num_gpu));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillByDefault(Return(num_cpu));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(num_core));

    m_default_characterization_str = "NODE_CHARACTERIZATION::CPU_CORE_FREQUENCY_EFFICIENT 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_0 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_1 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_10 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_11 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_12 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_13 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_14 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_2 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_3 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_4 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_5 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_6 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_7 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_8 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_9 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_EFFICIENT 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_0 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_1 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_10 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_11 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_12 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_13 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_14 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_2 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_3 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_4 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_5 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_6 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_7 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_8 0 0 0\n"
                                     "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_9 0 0 0\n"
                                     "NODE_CHARACTERIZATION::GPU_CORE_FREQUENCY_EFFICIENT 0 0 0\n";


    m_characterization_file_name = "PlatCharIOGroupTest-cache";
}

void PlatformCharacterizationIOGroupTest::TearDown()
{
    unlink(m_characterization_file_name.c_str());
}

TEST_F(PlatformCharacterizationIOGroupTest, valid_signals_and_controls)
{
    PlatformCharacterizationIOGroup nodechar_io(*m_platform_topo, m_characterization_file_name);
    for (const auto &sig : nodechar_io.signal_names()) {
        EXPECT_TRUE(nodechar_io.is_valid_signal(sig));
        EXPECT_NE(GEOPM_DOMAIN_INVALID, nodechar_io.signal_domain_type(sig));
        EXPECT_LT(-1, nodechar_io.signal_behavior(sig));

        // Every signal should have a control of the same name
        EXPECT_TRUE(nodechar_io.is_valid_control(sig));
        // Every signal & corrolary control should have the same domain type
        EXPECT_EQ(nodechar_io.signal_domain_type(sig), nodechar_io.control_domain_type(sig));
    }

    // Every signal having a control of the same name implies
    // there should be an equal number of signals and controls
    EXPECT_EQ(nodechar_io.control_names().size(), nodechar_io.signal_names().size());
}

TEST_F(PlatformCharacterizationIOGroupTest, read_default_signal)
{
    PlatformCharacterizationIOGroup nodechar_io(*m_platform_topo, m_characterization_file_name);
    for (const auto &sig : nodechar_io.signal_names()) {
        int domain_type = nodechar_io.signal_domain_type(sig);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            EXPECT_EQ(0, nodechar_io.read_signal(sig,
                                                 domain_type,
                                                 domain_idx));
        }
    }
}

TEST_F(PlatformCharacterizationIOGroupTest, read_signal)
{
    std::string characterization_str = "NODE_CHARACTERIZATION::CPU_CORE_FREQUENCY_EFFICIENT 0 0 1.45e9 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_0 0 0 223 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_1 0 0 212 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_10 0 0 920 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_11 0 0 181 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_12 0 0 617 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_13 0 0 151 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_14 0 0 314 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_2 0 0 121 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_3 0 0 101 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_4 0 0 789 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_5 0 0 456 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_6 0 0 123 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_7 0 0 321 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_8 0 0 654 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_9 0 0 987 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_EFFICIENT 0 0 2.22e+09 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_0 0 0 123 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_1 0 0 4 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_10 0 0 5 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_11 0 0 6 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_12 0 0 7 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_13 0 0 8 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_14 0 0 9 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_2 0 0 10 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_3 0 0 11 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_4 0 0 12 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_5 0 0 13 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_6 0 0 14 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_7 0 0 15 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_8 0 0 16 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_9 0 0 17 \n"
                                       "NODE_CHARACTERIZATION::GPU_CORE_FREQUENCY_EFFICIENT 0 0 1e+09 \n";

    write_characterization(characterization_str);
    PlatformCharacterizationIOGroup nodechar_io(*m_platform_topo, m_characterization_file_name);
    //No zero values
    for (const auto &sig : nodechar_io.signal_names()) {
        int domain_type = nodechar_io.signal_domain_type(sig);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            EXPECT_NE(0, nodechar_io.read_signal(sig,
                                                 domain_type,
                                                 domain_idx));
        }
    }

    std::map<std::string, double> sig_val_map = {{"NODE_CHARACTERIZATION::CPU_CORE_FREQUENCY_EFFICIENT", 1.45e9},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_0", 223},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_1", 212},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_10", 920},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_11", 181},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_12", 617},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_13", 151},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_14", 314},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_2", 121},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_3", 101},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_4", 789},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_5", 456},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_6", 123},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_7", 321},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_8", 654},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_9", 987},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_EFFICIENT", 2.22e09},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_0", 123},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_1", 4},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_10", 5},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_11", 6},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_12", 7},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_13", 8},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_14", 9},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_2", 10},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_3", 11},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_4", 12},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_5", 13},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_6", 14},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_7", 15},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_8", 16},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_9", 17},
                                                {"NODE_CHARACTERIZATION::GPU_CORE_FREQUENCY_EFFICIENT", 1e9}};

    for (const auto &sig : sig_val_map) {
        int domain_type = nodechar_io.signal_domain_type(sig.first);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            EXPECT_FLOAT_EQ(sig.second, nodechar_io.read_signal(sig.first,
                                                                domain_type,
                                                                domain_idx));
        }
    }

}

TEST_F(PlatformCharacterizationIOGroupTest, write_control_read_signal)
{
    PlatformCharacterizationIOGroup nodechar_io(*m_platform_topo, m_characterization_file_name);
    int sig_idx = 0;

    // Every signal should have a control of the same name,
    // so we use the signal name list to write a value for
    // each signal
    for (const auto &sig : nodechar_io.signal_names()) {
        int domain_type = nodechar_io.signal_domain_type(sig);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            // Expect 0 to start
            EXPECT_EQ(0, nodechar_io.read_signal(sig,
                                                 domain_type,
                                                 domain_idx));

            nodechar_io.write_control(sig, domain_type, domain_idx, domain_idx+sig_idx);
        }
        ++sig_idx;
    }

    sig_idx = 0;
    for (const auto &sig : nodechar_io.signal_names()) {
        int domain_type = nodechar_io.control_domain_type(sig);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            EXPECT_EQ(sig_idx + domain_idx, nodechar_io.read_signal(sig,
                                                                    domain_type,
                                                                    domain_idx));

        }
        ++sig_idx;
    }
}

TEST_F(PlatformCharacterizationIOGroupTest, push_control_adjust_write_batch)
{
    PlatformCharacterizationIOGroup nodechar_io(*m_platform_topo, m_characterization_file_name);
    int sig_idx = 99;
    std::map<int, double> batch_value;

    // setup batch values
    for (const auto &ctrl : nodechar_io.control_names()) {
        int domain_type = nodechar_io.control_domain_type(ctrl);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            batch_value[nodechar_io.push_control(ctrl, domain_type, domain_idx)] = sig_idx + domain_idx;
        }
        ++sig_idx;
    }

    // adjust
    for (auto& sv: batch_value) {
        EXPECT_NO_THROW(nodechar_io.adjust(sv.first, sv.second));
    }

    //Check results prior to write batch
    for (const auto &ctrl : nodechar_io.control_names()) {
        int domain_type = nodechar_io.control_domain_type(ctrl);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            EXPECT_EQ(0, nodechar_io.read_signal(ctrl,
                                                 domain_type,
                                                 domain_idx));
        }
    }

    EXPECT_NO_THROW(nodechar_io.write_batch());
    // Check results after write batch
    sig_idx = 99;
    for (const auto &ctrl : nodechar_io.control_names()) {
        int domain_type = nodechar_io.control_domain_type(ctrl);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            EXPECT_EQ(sig_idx + domain_idx, nodechar_io.read_signal(ctrl,
                                                                    domain_type,
                                                                    domain_idx));
        }
        ++sig_idx;
    }
}

TEST_F(PlatformCharacterizationIOGroupTest, read_signal_and_batch)
{
    std::string characterization_str = "NODE_CHARACTERIZATION::CPU_CORE_FREQUENCY_EFFICIENT 0 0 1.45e9 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_0 0 0 223 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_1 0 0 212 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_10 0 0 920 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_11 0 0 181 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_12 0 0 617 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_13 0 0 151 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_14 0 0 314 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_2 0 0 121 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_3 0 0 101 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_4 0 0 789 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_5 0 0 456 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_6 0 0 123 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_7 0 0 321 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_8 0 0 654 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_9 0 0 987 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_EFFICIENT 0 0 2.22e+09 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_0 0 0 123 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_1 0 0 4 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_10 0 0 5 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_11 0 0 6 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_12 0 0 7 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_13 0 0 8 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_14 0 0 9 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_2 0 0 10 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_3 0 0 11 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_4 0 0 12 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_5 0 0 13 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_6 0 0 14 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_7 0 0 15 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_8 0 0 16 \n"
                                       "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_9 0 0 17 \n"
                                       "NODE_CHARACTERIZATION::GPU_CORE_FREQUENCY_EFFICIENT 0 0 1e+09 \n";

    write_characterization(characterization_str);
    PlatformCharacterizationIOGroup nodechar_io(*m_platform_topo, m_characterization_file_name);

    std::map<std::string, int> batch_idx;
    for (const auto &sig : nodechar_io.signal_names()) {
        int domain_type = nodechar_io.signal_domain_type(sig);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            // Check that it's the non-default map specified
            EXPECT_NE(0, nodechar_io.read_signal(sig,
                                                 domain_type,
                                                 domain_idx));
            // save the batch id from push_signal
            batch_idx[sig] = nodechar_io.push_signal(sig, domain_type, domain_idx);
        }
    }
    nodechar_io.read_batch();

    //Expected values
    std::map<std::string, double> sig_val_map = {{"NODE_CHARACTERIZATION::CPU_CORE_FREQUENCY_EFFICIENT", 1.45e9},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_0", 223},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_1", 212},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_10", 920},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_11", 181},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_12", 617},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_13", 151},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_14", 314},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_2", 121},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_3", 101},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_4", 789},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_5", 456},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_6", 123},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_7", 321},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_8", 654},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_9", 987},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_EFFICIENT", 2.22e09},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_0", 123},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_1", 4},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_10", 5},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_11", 6},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_12", 7},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_13", 8},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_14", 9},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_2", 10},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_3", 11},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_4", 12},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_5", 13},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_6", 14},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_7", 15},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_8", 16},
                                                {"NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_9", 17},
                                                {"NODE_CHARACTERIZATION::GPU_CORE_FREQUENCY_EFFICIENT", 1e9}};

    for (const auto &sv : batch_idx) {
        double read_batch_val = nodechar_io.sample(sv.second);

        int domain_type = nodechar_io.signal_domain_type(sv.first);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            // Check read_signal provides expected value
            double read_signal_val =  nodechar_io.read_signal(sv.first, domain_type, domain_idx);
            EXPECT_EQ(sig_val_map[sv.first], read_signal_val);

            // Check that read_signal & read_batch/sample values match
            EXPECT_EQ(read_batch_val, read_signal_val);
        }
    }
}

//Test case: Error path testing
TEST_F(PlatformCharacterizationIOGroupTest, error_path)
{
    // Invalid signal, invalid string format
    std::string bad_str = "FOO BAR BAZ IS AN INVALID STRING";
    write_characterization(bad_str);
    GEOPM_EXPECT_THROW_MESSAGE(PlatformCharacterizationIOGroup
                               nodechar_io(*m_platform_topo, m_characterization_file_name),
                               GEOPM_ERROR_RUNTIME,
                               "Invalid characterization line");

    // Invalid signal, valid string format <SIGNAL> <DOMAIN> <DOMAIN_IDX> <VALUE>
    bad_str = "NIDA_CHERICTUROZUTEAN::CPY_YNCARO_MAXAMOM_MIMURY_BYNDWODTH_11 0 0 6";
    write_characterization(bad_str);
    GEOPM_EXPECT_THROW_MESSAGE(PlatformCharacterizationIOGroup
                               nodechar_io(*m_platform_topo, m_characterization_file_name),
                               GEOPM_ERROR_RUNTIME,
                               "Invalid characterization line");

    // Valid signal, invalid domain size
    bad_str = "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_9 0 10000000 17";
    write_characterization(bad_str);
    GEOPM_EXPECT_THROW_MESSAGE(PlatformCharacterizationIOGroup
                               nodechar_io(*m_platform_topo, m_characterization_file_name),
                               GEOPM_ERROR_RUNTIME,
                               "Invalid characterization line");

    // Valid signal, Invalid domain
    bad_str = "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_9 9999 0 17";
    write_characterization(bad_str);
    GEOPM_EXPECT_THROW_MESSAGE(PlatformCharacterizationIOGroup
                               nodechar_io(*m_platform_topo, m_characterization_file_name),
                               GEOPM_ERROR_RUNTIME,
                               "Invalid characterization line");

    // Construct the IOGroup without error
    write_characterization(m_default_characterization_str);
    PlatformCharacterizationIOGroup nodechar_io(*m_platform_topo, m_characterization_file_name);

    // Setup read batch
    std::map<std::string, int> batch_idx;
    for (const auto &sig : nodechar_io.signal_names()) {
        int domain_type = nodechar_io.signal_domain_type(sig);
        int num_domain = m_platform_topo->num_domain(domain_type);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            // Check that it's the default map specified
            EXPECT_EQ(0, nodechar_io.read_signal(sig,
                                                 domain_type,
                                                 domain_idx));
            // save the batch id from push_signal
            batch_idx[sig] = nodechar_io.push_signal(sig, domain_type, domain_idx);
        }
    }

    // sample batch idx prior to read_batch
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.sample(0),
                               GEOPM_ERROR_INVALID,
                               "signal has not been read");

    // sample batch idx out of range
    nodechar_io.read_batch();
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.sample(batch_idx.size()),
                               GEOPM_ERROR_INVALID,
                               "out of range");

    // adjust out of range - prior to any settings
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.adjust(0, -1),
                               GEOPM_ERROR_INVALID,
                               "out of range");

    // read invalid signal
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.read_signal("INVALID", 0, 0),
                               GEOPM_ERROR_INVALID,
                               "not valid for PlatformCharacterizationIOGroup");

    // read valid signal, invalid domain
    std::string valid_sig = "NODE_CHARACTERIZATION::GPU_CORE_FREQUENCY_EFFICIENT";
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.read_signal(valid_sig, GEOPM_DOMAIN_INVALID, 0),
                               GEOPM_ERROR_INVALID,
                               "domain_type must be");

    // read valid signal, valid domain, invalid domain idx
    int domain_type = nodechar_io.signal_domain_type(valid_sig);
    int num_domain = m_platform_topo->num_domain(domain_type);
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.read_signal(valid_sig, domain_type, num_domain),
                               GEOPM_ERROR_INVALID,
                               "domain_idx out of range.");

    // Push invalid signal
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.push_signal("INVALID", 0, 0),
                               GEOPM_ERROR_INVALID,
                               "not valid for PlatformCharacterizationIOGroup");

    // Push valid signal, invalid domain
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.push_signal(valid_sig, GEOPM_DOMAIN_INVALID, 0),
                               GEOPM_ERROR_INVALID,
                               "domain_type must be");

    // Push valid signal, valid domain, invalid domain idx
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.push_signal(valid_sig, domain_type, num_domain),
                               GEOPM_ERROR_INVALID,
                               "domain_idx out of range.");

    // Push invalid control
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.push_control("INVALID", 0, 0),
                               GEOPM_ERROR_INVALID,
                               "not valid for PlatformCharacterizationIOGroup");

    // Push valid control, invalid domain
    std::string valid_ctrl = "NODE_CHARACTERIZATION::GPU_CORE_FREQUENCY_EFFICIENT";
    domain_type = nodechar_io.signal_domain_type(valid_ctrl);
    num_domain = m_platform_topo->num_domain(domain_type);
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.push_control(valid_ctrl, GEOPM_DOMAIN_INVALID, num_domain),
                               GEOPM_ERROR_INVALID,
                               "domain_type must be");

    // Push valid control, valid domain, invalid domain idx
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.push_control(valid_ctrl, domain_type, num_domain),
                               GEOPM_ERROR_INVALID,
                               "domain_idx out of range.");

    // write invalid control
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.write_control("INVALID", 0, 0, -12345),
                               GEOPM_ERROR_INVALID,
                               "not valid for PlatformCharacterizationIOGroup");

    // write valid control, invalid domain
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.write_control(valid_ctrl, GEOPM_DOMAIN_INVALID, 0, -12345),
                               GEOPM_ERROR_INVALID,
                               "domain_type must be");

    // write valid control, valid domain, invalid domain idx
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.write_control(valid_ctrl, domain_type, num_domain, -12345),
                               GEOPM_ERROR_INVALID,
                               "domain_idx out of range.");

    // Invalid signal tests
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.agg_function("INALID"),
                               GEOPM_ERROR_INVALID,
                               "not valid for PlatformCharacterizationIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.format_function("INALID"),
                               GEOPM_ERROR_INVALID,
                               "not valid for PlatformCharacterizationIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.signal_description("INALID"),
                               GEOPM_ERROR_INVALID,
                               "not valid for PlatformCharacterizationIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.control_description("INALID"),
                               GEOPM_ERROR_INVALID,
                               "not valid for PlatformCharacterizationIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(nodechar_io.signal_behavior("INALID"),
                               GEOPM_ERROR_INVALID,
                               "not valid for PlatformCharacterizationIOGroup");
}

TEST_F(PlatformCharacterizationIOGroupTest, check_file_too_old)
{
    write_characterization(m_default_characterization_str);

    struct sysinfo si;
    sysinfo(&si);
    struct geopm_time_s current_time;
    geopm_time_real(&current_time);
    unsigned int last_boot_time = current_time.t.tv_sec - si.uptime;

    // Modify the last modified time to be prior to the last boot
    unsigned int old_time = last_boot_time - 600; // 10 minutes before boot
    struct utimbuf file_times = {old_time, old_time};
    utime(m_characterization_file_name.c_str(), &file_times);

    // Verify the modification worked
    struct stat file_stat;
    stat(m_characterization_file_name.c_str(), &file_stat);
    ASSERT_EQ(old_time, file_stat.st_mtime);

    PlatformCharacterizationIOGroup nodechar_io(*m_platform_topo, m_characterization_file_name);

    // Verify the cache was regenerated because it was too old
    stat(m_characterization_file_name.c_str(), &file_stat);
    ASSERT_LT(last_boot_time, file_stat.st_mtime);

    // Verify the new file contents
    std::string new_file_contents = geopm::read_file(m_characterization_file_name);
    ASSERT_EQ(m_default_characterization_str, new_file_contents);
}

TEST_F(PlatformCharacterizationIOGroupTest, check_file_bad_perms)
{
    write_characterization(m_default_characterization_str);

    // Override the permissions to a known bad state: 0o644
    mode_t bad_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    chmod(m_characterization_file_name.c_str(), bad_perms);

    // Verify initial state
    struct stat file_stat;
    stat(m_characterization_file_name.c_str(), &file_stat);
    mode_t actual_perms = file_stat.st_mode & ~S_IFMT;
    ASSERT_EQ(bad_perms, actual_perms);

    PlatformCharacterizationIOGroup nodechar_io(*m_platform_topo, m_characterization_file_name);

    // Verify that the cache was regenerated because it had the wrong permissions
    stat(m_characterization_file_name.c_str(), &file_stat);
    mode_t expected_perms = S_IRUSR | S_IWUSR; // 0o600 by default
    actual_perms = file_stat.st_mode & ~S_IFMT;
    ASSERT_EQ(expected_perms, actual_perms);

    // Verify the new file contents
    std::string new_file_contents = geopm::read_file(m_characterization_file_name);
    ASSERT_EQ(m_default_characterization_str, new_file_contents);
}

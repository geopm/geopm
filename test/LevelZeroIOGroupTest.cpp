/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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
#include <limits.h>

#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "config.h"
#include "Helper.hpp"
#include "Exception.hpp"
#include "PlatformTopo.hpp"
#include "PluginFactory.hpp"
#include "LevelZeroIOGroup.hpp"
#include "geopm_test.hpp"
#include "MockLevelZeroDevicePool.hpp"
#include "MockLevelZero.hpp"
#include "MockPlatformTopo.hpp"

using geopm::LevelZeroIOGroup;
using geopm::PlatformTopo;
using geopm::Exception;
using testing::Return;

class LevelZeroIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void write_affinitization(const std::string &affinitization_str);

        std::unique_ptr<MockPlatformTopo> m_platform_topo;
        std::shared_ptr<MockLevelZeroDevicePool> m_device_pool;
};

void LevelZeroIOGroupTest::SetUp()
{
    const int num_board = 1;
    const int num_package = 2;
    const int num_board_accelerator = 4;
    const int num_board_accelerator_subdevice = 8;
    const int num_core = 20;
    const int num_cpu = 40;

    m_device_pool = std::make_shared<MockLevelZeroDevicePool>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();

    //Platform Topo prep
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(num_board));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(num_package));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR))
        .WillByDefault(Return(num_board_accelerator));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP))
        .WillByDefault(Return(num_board_accelerator_subdevice));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillByDefault(Return(num_cpu));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(num_core));

    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        if (cpu_idx < 10) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(0));
        }
        else if (cpu_idx < 20) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(1));
        }
        else if (cpu_idx < 30) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(2));
        }
        else {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(3));
        }
    }

    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        if (cpu_idx < 5) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(0));
        }
        else if (cpu_idx < 10) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(1));
        }
        else if (cpu_idx < 15) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(2));
        }
        else if (cpu_idx < 20) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(3));
        }
        else if (cpu_idx < 25) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(4));
        }
        else if (cpu_idx < 30) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(5));
        }
        else if (cpu_idx < 35) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(6));
        }
        else {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_BOARD_ACCELERATOR, cpu_idx))
                .WillByDefault(Return(7));
        }
    }


    EXPECT_CALL(*m_device_pool, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR)).WillRepeatedly(Return(num_board_accelerator));
    EXPECT_CALL(*m_device_pool, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)).WillRepeatedly(Return(num_board_accelerator_subdevice));
}

void LevelZeroIOGroupTest::TearDown()
{
}

TEST_F(LevelZeroIOGroupTest, valid_signals)
{
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool);
    for (const auto &sig : levelzero_io.signal_names()) {
        EXPECT_TRUE(levelzero_io.is_valid_signal(sig));
        EXPECT_NE(GEOPM_DOMAIN_INVALID, levelzero_io.signal_domain_type(sig));
        EXPECT_LT(-1, levelzero_io.signal_behavior(sig));
    }
}

TEST_F(LevelZeroIOGroupTest, push_control_adjust_write_batch)
{
    const int num_accelerator_subdevice = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP);
    std::map<int, double> batch_value;
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool);

    std::vector<double> mock_freq = {1530, 1320, 420, 135, 1620, 812, 199, 1700};

    for (int sub_idx = 0; sub_idx < num_accelerator_subdevice; ++sub_idx) {
        batch_value[(levelzero_io.push_control("LEVELZERO::FREQUENCY_GPU_CONTROL",
                                        GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx))] = mock_freq.at(sub_idx)*1e6;
        batch_value[(levelzero_io.push_control("FREQUENCY_ACCELERATOR_CONTROL",
                                        GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx))] = mock_freq.at(sub_idx)*1e6;
        EXPECT_CALL(*m_device_pool,
                    frequency_control(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE, mock_freq.at(sub_idx))).Times(2);
    }

    for (auto& sv: batch_value) {
        // Given that we are mocking LEVELZERODevicePool the actual setting here doesn't matter
        EXPECT_NO_THROW(levelzero_io.adjust(sv.first, sv.second));
    }
    EXPECT_NO_THROW(levelzero_io.write_batch());
}

TEST_F(LevelZeroIOGroupTest, write_control)
{
    const int num_accelerator_subdevice = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP);
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool);

    std::vector<double> mock_freq = {1530, 1320, 420, 135, 900, 9001, 8010, 4500};

    for (int sub_idx = 0; sub_idx < num_accelerator_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool,
                    frequency_control(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE, mock_freq.at(sub_idx))).Times(2);

        EXPECT_NO_THROW(levelzero_io.write_control("LEVELZERO::FREQUENCY_GPU_CONTROL",
                                              GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx,
                                              mock_freq.at(sub_idx)*1e6));

        EXPECT_NO_THROW(levelzero_io.write_control("FREQUENCY_ACCELERATOR_CONTROL",
                                              GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx,
                                              mock_freq.at(sub_idx)*1e6));
    }
}

TEST_F(LevelZeroIOGroupTest, read_signal_and_batch)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);
    const int num_accelerator_subdevice = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP);

    std::vector<double> mock_freq = {1530, 1630, 1320, 1420, 420, 520, 135, 235};
    std::vector<double> mock_energy = {9000000, 11000000, 2300000, 5341000000};
    std::vector<int> batch_idx;

    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool);

    for (int sub_idx = 0; sub_idx < num_accelerator_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq.at(sub_idx)));
        batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx));
    }

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy.at(accel_idx)));
        batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::ENERGY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx));
    }


    levelzero_io.read_batch();
    for (int sub_idx = 0; sub_idx < num_accelerator_subdevice; ++sub_idx) {
        double frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        double frequency_batch = levelzero_io.sample(batch_idx.at(sub_idx));

        EXPECT_DOUBLE_EQ(frequency, mock_freq.at(sub_idx)*1e6);
        EXPECT_DOUBLE_EQ(frequency, frequency_batch);
    }

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        double energy = levelzero_io.read_signal("LEVELZERO::ENERGY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        double energy_batch = levelzero_io.sample(batch_idx.at(num_accelerator_subdevice+accel_idx));

        EXPECT_DOUBLE_EQ(energy, mock_energy.at(accel_idx)/1e6);
        EXPECT_DOUBLE_EQ(energy, energy_batch);
    }

    //second round of testing with a modified value
    mock_freq = {1730, 1830, 1520, 1620, 620, 720, 335, 435};
    mock_energy = {9320000, 12300000, 2360000, 3417000000};
    for (int sub_idx = 0; sub_idx < num_accelerator_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq.at(sub_idx)));
    }
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy.at(accel_idx)));
    }

    levelzero_io.read_batch();
    for (int sub_idx = 0; sub_idx < num_accelerator_subdevice; ++sub_idx) {
        double frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        double frequency_batch = levelzero_io.sample(batch_idx.at(sub_idx));

        EXPECT_DOUBLE_EQ(frequency, mock_freq.at(sub_idx)*1e6);
        EXPECT_DOUBLE_EQ(frequency, frequency_batch);
    }
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        double energy = levelzero_io.read_signal("LEVELZERO::ENERGY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        double energy_batch = levelzero_io.sample(batch_idx.at(num_accelerator_subdevice+accel_idx));

        EXPECT_DOUBLE_EQ(energy, mock_energy.at(accel_idx)/1e6);
        EXPECT_DOUBLE_EQ(energy, energy_batch);
    }
}

TEST_F(LevelZeroIOGroupTest, read_signal)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);
    const int num_accelerator_subdevice = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP);

    //Frequency
    std::vector<double> mock_freq_gpu = {1530, 1320, 420, 135, 900, 927, 293, 400};
    std::vector<double> mock_freq_mem = {130, 1020, 200, 150, 300, 442, 782, 1059};
    std::vector<double> mock_freq_min_gpu = {200, 320, 400, 350, 111, 222, 333, 444};
    std::vector<double> mock_freq_max_gpu = {2000, 3200, 4200, 1350, 555, 666, 777, 888};
    std::vector<double> mock_freq_min_mem = {100, 220, 300, 450, 999, 1010, 1111, 1212};
    std::vector<double> mock_freq_max_mem = {1000, 2200, 3200, 1450, 1313, 1414, 1515, 1616};
    //Active time
    std::vector<uint64_t> mock_active_time = {123, 970, 550, 20, 52, 567, 888, 923};
    std::vector<uint64_t> mock_active_time_timestamp = {182, 970, 650, 33, 283, 331, 675, 9000};
    std::vector<uint64_t> mock_active_time_compute = {1, 90, 50, 0, 123, 144, 521, 445};
    std::vector<uint64_t> mock_active_time_timestamp_compute = {12, 90, 150, 3, 772, 248, 932, 122};
    std::vector<uint64_t> mock_active_time_copy = {12, 20, 30, 40, 44, 55, 66, 77};
    std::vector<uint64_t> mock_active_time_timestamp_copy = {50, 60, 53, 55, 66, 77, 88, 99};
    //Power & energy
    std::vector<int32_t> mock_power_limit_min = {30000, 80000, 20000, 70000};
    std::vector<int32_t> mock_power_limit_max = {310000, 280000, 320000, 270000};
    std::vector<int32_t> mock_power_limit_tdp = {320000, 290000, 330000, 280000};
    std::vector<uint64_t> mock_energy = {630000000, 280000000, 470000000, 950000000};
    std::vector<uint64_t> mock_energy_timestamp = {153, 70, 300, 50};

    for (int sub_idx = 0; sub_idx < num_accelerator_subdevice; ++sub_idx) {
        //Frequency
        EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq_gpu.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_freq_mem.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_min(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq_min_gpu.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_max(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq_max_gpu.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_min(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_freq_min_mem.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_max(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_freq_max_mem.at(sub_idx)));

        //Active time
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_active_time.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_active_time_timestamp.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_active_time_compute.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_active_time_timestamp_compute.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_active_time_copy.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_active_time_timestamp_copy.at(sub_idx)));
    }

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        //Power & energy
        EXPECT_CALL(*m_device_pool, power_limit_min(GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_power_limit_min.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_max(GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_power_limit_max.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_tdp(GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_power_limit_tdp.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, energy_timestamp(GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy_timestamp.at(accel_idx)));
    }

    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool);

    for (int sub_idx = 0; sub_idx < num_accelerator_subdevice; ++sub_idx) {
        //Frequency
        double frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        double frequency_alias = levelzero_io.read_signal("FREQUENCY_ACCELERATOR", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, frequency_alias);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_gpu.at(sub_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_MEMORY", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_mem.at(sub_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_MIN_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_min_gpu.at(sub_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_MAX_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_max_gpu.at(sub_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_MIN_MEMORY", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_min_mem.at(sub_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_MAX_MEMORY", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_max_mem.at(sub_idx)*1e6);

        //Active time
        double active_time = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(active_time, mock_active_time.at(sub_idx)/1e6);
        double active_time_timestamp = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_TIMESTAMP", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(active_time_timestamp, mock_active_time_timestamp.at(sub_idx)/1e6);
        double active_time_compute = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_COMPUTE", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(active_time_compute, mock_active_time_compute.at(sub_idx)/1e6);
        double active_time_timestamp_compute = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_TIMESTAMP_COMPUTE", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(active_time_timestamp_compute, mock_active_time_timestamp_compute.at(sub_idx)/1e6);
        double active_time_copy = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_COPY", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(active_time_copy, mock_active_time_copy.at(sub_idx)/1e6);
        double active_time_timestamp_copy = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_TIMESTAMP_COPY", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(active_time_timestamp_copy, mock_active_time_timestamp_copy.at(sub_idx)/1e6);
    }

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        //Power & energy
        double power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_MIN", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_min.at(accel_idx)/1e3);
        power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_MAX", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_max.at(accel_idx)/1e3);
        power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_DEFAULT", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_tdp.at(accel_idx)/1e3);

        double energy = levelzero_io.read_signal("LEVELZERO::ENERGY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(energy, mock_energy.at(accel_idx)/1e6);
        double energy_timestamp = levelzero_io.read_signal("LEVELZERO::ENERGY_TIMESTAMP", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(energy_timestamp, mock_energy_timestamp.at(accel_idx)/1e6);

    }

    // Assume DerivativeSignals class functions as expected
    // Just check validity of derived signals
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::POWER"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::UTILIZATION"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::UTILIZATION_COMPUTE"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::UTILIZATION_COPY"));
}

//Test case: Error path testing including:
//              - Attempt to push a signal at an invalid domain level
//              - Attempt to push an invalid signal
//              - Attempt to sample without a read_batch prior
//              - Attempt to read a signal at an invalid domain level
//              - Attempt to push a control at an invalid domain level
//              - Attempt to adjust a non-existent batch index
//              - Attempt to write a control at an invalid domain level
TEST_F(LevelZeroIOGroupTest, error_path)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);
    const int num_accelerator_subdevice = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP);

    std::vector<int> batch_idx;

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, accel_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq.at(accel_idx)));
    }
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool);

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.sample(0),
                               GEOPM_ERROR_INVALID, "batch_idx 0 out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_signal("LEVELZERO::INVALID", GEOPM_DOMAIN_BOARD_ACCELERATOR, 0),
                               GEOPM_ERROR_INVALID, "signal_name LEVELZERO::INVALID not valid for LevelZeroIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::INVALID", GEOPM_DOMAIN_BOARD_ACCELERATOR, 0),
                               GEOPM_ERROR_INVALID, "LEVELZERO::INVALID not valid for LevelZeroIOGroup");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_control("LEVELZERO::FREQUENCY_GPU_CONTROL", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.adjust(0, 12345.6),
                               GEOPM_ERROR_INVALID, "batch_idx 0 out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.write_control("LEVELZERO::FREQUENCY_GPU_CONTROL", GEOPM_DOMAIN_BOARD, 0, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_type must be");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_control("LEVELZERO::INVALID", GEOPM_DOMAIN_BOARD_ACCELERATOR, 0),
                               GEOPM_ERROR_INVALID, "control_name LEVELZERO::INVALID not valid for LevelZeroIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.write_control("LEVELZERO::INVALID", GEOPM_DOMAIN_BOARD_ACCELERATOR, 0, 1530000000),
                               GEOPM_ERROR_INVALID, "LEVELZERO::INVALID not valid for LevelZeroIOGroup");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, num_accelerator_subdevice),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, num_accelerator_subdevice),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_control("LEVELZERO::FREQUENCY_GPU_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, num_accelerator_subdevice),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_control("LEVELZERO::FREQUENCY_GPU_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.write_control("LEVELZERO::FREQUENCY_GPU_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, num_accelerator_subdevice, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.write_control("LEVELZERO::FREQUENCY_GPU_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, -1, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
}

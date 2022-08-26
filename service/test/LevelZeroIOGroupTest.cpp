/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <unistd.h>
#include <limits.h>

#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "config.h"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PluginFactory.hpp"
#include "LevelZeroIOGroup.hpp"
#include "geopm_test.hpp"
#include "MockLevelZeroDevicePool.hpp"
#include "MockLevelZero.hpp"
#include "MockPlatformTopo.hpp"
#include "MockSaveControl.hpp"

using geopm::LevelZeroIOGroup;
using geopm::PlatformTopo;
using geopm::Exception;
using testing::Return;
using testing::Throw;
using testing::AtLeast;
using testing::_;
using testing::StrictMock;

class LevelZeroIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void SetUpDefaultExpectCalls();
        void TearDown();
        void write_affinitization(const std::string &affinitization_str);

        std::unique_ptr<MockPlatformTopo> m_platform_topo;
        std::shared_ptr<MockLevelZeroDevicePool> m_device_pool;
        std::shared_ptr<MockSaveControl> m_mock_save_ctl;

        const int m_num_gpu = 4;
        const int m_num_gpu_subdevice = 8;
};

void LevelZeroIOGroupTest::SetUp()
{
    const int num_board = 1;
    const int num_package = 2;
    const int num_core = 20;
    const int num_cpu = 40;

    m_device_pool = std::make_shared<StrictMock<MockLevelZeroDevicePool> >();
    m_platform_topo = geopm::make_unique<StrictMock<MockPlatformTopo> >();
    m_mock_save_ctl = std::make_shared<StrictMock<MockSaveControl> >();

    //Platform Topo prep
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(num_board));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(num_package));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
        .WillByDefault(Return(m_num_gpu));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU_CHIP))
        .WillByDefault(Return(m_num_gpu_subdevice));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillByDefault(Return(num_cpu));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(num_core));

    EXPECT_CALL(*m_platform_topo, num_domain(_)).Times(AtLeast(0));

    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        if (cpu_idx < 10) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(0));
        }
        else if (cpu_idx < 20) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(1));
        }
        else if (cpu_idx < 30) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(2));
        }
        else {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(3));
        }
    }

    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        if (cpu_idx < 5) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(0));
        }
        else if (cpu_idx < 10) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(1));
        }
        else if (cpu_idx < 15) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(2));
        }
        else if (cpu_idx < 20) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(3));
        }
        else if (cpu_idx < 25) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(4));
        }
        else if (cpu_idx < 30) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(5));
        }
        else if (cpu_idx < 35) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(6));
        }
        else {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(7));
        }
    }

    EXPECT_CALL(*m_device_pool, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(m_num_gpu));
    EXPECT_CALL(*m_device_pool, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(m_num_gpu_subdevice));
}

void LevelZeroIOGroupTest::SetUpDefaultExpectCalls()
{
    // Expectations for signal/control pruning code in the constructor
    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, // GPU_ACTIVE_TIME
                    active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_ACTIVE_TIME_TIMESTAMP
                    active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_ACTIVE_TIME
                    active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_ACTIVE_TIME_TIMESTAMP
                    active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_FREQUENCY_MAX_AVAIL
                    frequency_max(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        // GPU_CORE_FREQUENCY_MAX_CONTROL (signal pruning), GPU_CORE_FREQUENCY_MIN_CONTROL (signal pruning),
        // the save_control() call, GPU_CORE_FREQUENCY_MAX_CONTROL (control pruning) * 2,
        // and GPU_CORE_FREQUENCY_MIN_CONTROL (control pruning) * 2 = 7 times
        EXPECT_CALL(*m_device_pool,
                    frequency_range(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).Times(7);
        EXPECT_CALL(*m_device_pool, // GPU_CORE_FREQUENCY_MIN_AVAIL
                    frequency_min(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_FREQUENCY_STATUS
                    frequency_status(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_FREQUENCY_EFFICIENT
                    frequency_efficient(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_THROTTLE_REASONS
                    frequency_throttle_reasons(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_UNCORE_ACTIVE_TIME
                    active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));
        EXPECT_CALL(*m_device_pool, // GPU_UNCORE_ACTIVE_TIME_TIMESTAMP
                    active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));
        EXPECT_CALL(*m_device_pool, // GPU_UNCORE_FREQUENCY_MAX_AVAIL
                    frequency_max(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));
        EXPECT_CALL(*m_device_pool, // GPU_UNCORE_FREQUENCY_MIN_AVAIL
                    frequency_min(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));
        EXPECT_CALL(*m_device_pool, // GPU_UNCORE_FREQUENCY_STATUS
                    frequency_status(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));
        EXPECT_CALL(*m_device_pool, // GPU_ENERGY
                    energy(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_ENERGY_TIMESTAMP
                    energy_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_PERFORMANCE_FACTOR
                    performance_factor(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).Times(2);

        // control pruning expectations
        // GPU_CORE_FREQUENCY_MAX_CONTROL, GPU_CORE_FREQUENCY_MIN_CONTROL, and the restore_control() direct call.
        EXPECT_CALL(*m_device_pool,
                    frequency_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE,
                                      0, 0)).Times(3);
        EXPECT_CALL(*m_device_pool, // GPU_CORE_PERFORMANCE_FACTOR_CONTROL
                    performance_factor_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE, 0));

    }

    // Expectations for signal pruning code in the constructor
    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, // GPU_ENERGY
                    energy(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_ENERGY_TIMESTAMP
                    energy_timestamp(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_POWER_LIMIT_DEFAULT
                    power_limit_tdp(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_POWER_LIMIT_MAX_AVAIL
                    power_limit_max(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_POWER_LIMIT_MIN_AVAIL
                    power_limit_min(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL));
    }
}

void LevelZeroIOGroupTest::TearDown()
{
}

TEST_F(LevelZeroIOGroupTest, valid_signals)
{
    SetUpDefaultExpectCalls();
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, nullptr);
    for (const auto &sig : levelzero_io.signal_names()) {
        EXPECT_TRUE(levelzero_io.is_valid_signal(sig));
        EXPECT_NE(GEOPM_DOMAIN_INVALID, levelzero_io.signal_domain_type(sig));
        EXPECT_LT(-1, levelzero_io.signal_behavior(sig));
    }
}


TEST_F(LevelZeroIOGroupTest, save_restore)
{
    SetUpDefaultExpectCalls();
    std::map<int, double> batch_value;
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, nullptr);

    std::vector<std::pair<double,double> > mock_freq_range = {{0, 1530}, {1000, 1320}, {30, 420}, {130, 135},
                                                              {20, 400}, {53, 123}, {1600, 1700}, {500, 500}};

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, // save_control caches current values
                    frequency_range(GEOPM_DOMAIN_GPU_CHIP, sub_idx, geopm::LevelZero::M_DOMAIN_COMPUTE)).
                    WillOnce(Return(mock_freq_range.at(sub_idx)));

        EXPECT_CALL(*m_device_pool, // restore_control restores cached values
                    frequency_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE,
                                      mock_freq_range.at(sub_idx).first, mock_freq_range.at(sub_idx).second));
    }

    levelzero_io.save_control();
    levelzero_io.restore_control();
}

TEST_F(LevelZeroIOGroupTest, push_control_adjust_write_batch)
{
    SetUpDefaultExpectCalls();
    std::map<int, double> batch_value;
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, nullptr);

    std::vector<double> mock_freq = {1530, 1320, 420, 135, 1620, 812, 199, 1700};

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        // When calling write_control() to write a new MAX, the MIN will be read first.
        // This MIN will be written a max.  The opposite occurs when writing MIN.
        EXPECT_CALL(*m_device_pool,
                    frequency_range(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).
                    WillRepeatedly(Return(std::make_pair(geopm::Agg::min(mock_freq), geopm::Agg::max(mock_freq))));

        batch_value[(levelzero_io.push_control("LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL",
                                        GEOPM_DOMAIN_GPU_CHIP, sub_idx))] = mock_freq.at(sub_idx) * 1e6;
        batch_value[(levelzero_io.push_control("GPU_CORE_FREQUENCY_MAX_CONTROL",
                                        GEOPM_DOMAIN_GPU_CHIP, sub_idx))] = mock_freq.at(sub_idx) * 1e6;

        // Only 1 call to frequency_control is expected even though 2 controls were pushed:
        //  push_control() has logic to see if a control was already pushed, including aliased controls.
        //  If it has already been pushed, a subsequent push is a no-op and will return the previous index.
        EXPECT_CALL(*m_device_pool,
                    frequency_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE,
                                      geopm::Agg::min(mock_freq), mock_freq.at(sub_idx))).Times(1);
    }

    for (auto& sv: batch_value) { // batch_value will have a size of m_num_gpu_subdevice (note: not 2 * m_num_gpu_subdevice)
        // Given that we are mocking LEVELZERODevicePool the actual setting here doesn't matter
        EXPECT_NO_THROW(levelzero_io.adjust(sv.first, sv.second));
    }
    EXPECT_NO_THROW(levelzero_io.write_batch());
}

TEST_F(LevelZeroIOGroupTest, write_control)
{
    SetUpDefaultExpectCalls();
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, nullptr);

    std::vector<double> mock_freq = {1530, 1320, 420, 135, 900, 9001, 8010, 4500};

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {

        EXPECT_CALL(*m_device_pool,
                    frequency_range(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).
                    WillRepeatedly(Return(std::make_pair(geopm::Agg::min(mock_freq), geopm::Agg::max(mock_freq))));

        EXPECT_CALL(*m_device_pool,
                    frequency_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE,
                                      geopm::Agg::min(mock_freq), mock_freq.at(sub_idx))).Times(2);

        EXPECT_NO_THROW(levelzero_io.write_control("LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL",
                                              GEOPM_DOMAIN_GPU_CHIP, sub_idx,
                                              mock_freq.at(sub_idx)*1e6));

        EXPECT_NO_THROW(levelzero_io.write_control("GPU_CORE_FREQUENCY_MAX_CONTROL",
                                              GEOPM_DOMAIN_GPU_CHIP, sub_idx,
                                              mock_freq.at(sub_idx)*1e6));
    }
}

TEST_F(LevelZeroIOGroupTest, read_signal_and_batch)
{
    SetUpDefaultExpectCalls();
    std::vector<double> mock_freq = {1530, 1630, 1320, 1420, 420, 520, 135, 235};
    std::vector<double> mock_freq_efficient = {700, 800, 600, 499, 300, 250, 99, 200};
    std::vector<double> mock_throttle = {0, 2, 4, 10, 1, 3, 9, 5};
    std::vector<double> mock_energy = {9000000, 11000000, 2300000, 5341000000};
    std::vector<double> mock_time = {10, 20.220, 42.2, 100.3333333};
    std::vector<double> mock_time_chip = {5, 10.110, 21.1, 100.55553,
                                          6, 12, 22, 15};
    std::vector<double> mock_energy_chip = {4000000, 5000000, 1100000, 2621000000,
                                            4200000, 5200000, 1120000, 2621200000,};
    std::vector<int> batch_idx;

    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, nullptr);

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq.at(sub_idx)));
        batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, sub_idx));
    }

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, frequency_throttle_reasons(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_throttle.at(sub_idx)));
        batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_CORE_THROTTLE_REASONS", GEOPM_DOMAIN_GPU_CHIP, sub_idx));
    }

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy_chip.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, energy_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_time_chip.at(sub_idx)));
        batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_CORE_ENERGY", GEOPM_DOMAIN_GPU_CHIP, sub_idx));
    }

    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy.at(gpu_idx)));
        // Since GPU_ENERGY is in m_special_signal_set, GPU_ENERGY_TIMESTAMP is automatically pushed under the hood.
        EXPECT_CALL(*m_device_pool, energy_timestamp(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_time.at(gpu_idx)));
        batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_ENERGY", GEOPM_DOMAIN_GPU, gpu_idx));
    }

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, frequency_efficient(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq_efficient.at(sub_idx)));
        batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, sub_idx));
    }

    levelzero_io.read_batch();

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        double frequency = levelzero_io.read_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        double frequency_batch = levelzero_io.sample(batch_idx.at(sub_idx));
        EXPECT_DOUBLE_EQ(frequency, mock_freq.at(sub_idx)*1e6);
        EXPECT_DOUBLE_EQ(frequency, frequency_batch);

        double throttle = levelzero_io.read_signal("LEVELZERO::GPU_CORE_THROTTLE_REASONS", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        double throttle_batch = levelzero_io.sample(batch_idx.at(sub_idx + m_num_gpu_subdevice));
        EXPECT_DOUBLE_EQ(throttle, mock_throttle.at(sub_idx));
        EXPECT_DOUBLE_EQ(throttle, throttle_batch);

        double energy_chip = levelzero_io.read_signal("LEVELZERO::GPU_CORE_ENERGY", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        double energy_chip_batch = levelzero_io.sample(batch_idx.at(sub_idx + 2 * m_num_gpu_subdevice));

        EXPECT_DOUBLE_EQ(energy_chip, mock_energy_chip.at(sub_idx)/1e6);
        EXPECT_DOUBLE_EQ(energy_chip, energy_chip_batch);
    }

    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        double energy = levelzero_io.read_signal("LEVELZERO::GPU_ENERGY", GEOPM_DOMAIN_GPU, gpu_idx);
        double energy_batch = levelzero_io.sample(batch_idx.at(3 * m_num_gpu_subdevice + gpu_idx));
        EXPECT_DOUBLE_EQ(energy, mock_energy.at(gpu_idx)/1e6);
        EXPECT_DOUBLE_EQ(energy, energy_batch);
    }

    //second round of testing with a modified value
    mock_freq = {1730, 1830, 1520, 1620, 620, 720, 335, 435};
    mock_throttle = {2, 6, 8, 4, 12, 16, 18, 22};
    mock_energy = {9320000, 12300000, 2360000, 3417000000};
    mock_energy_chip = {4310000, 11100000, 1240000, 1655000000,
                        4300000, 11000000, 1230000, 1555000000};
    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_throttle_reasons(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_throttle.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy_chip.at(sub_idx)));
    }

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, frequency_throttle_reasons(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_throttle.at(sub_idx)));
    }

    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy.at(gpu_idx)));
    }

    levelzero_io.read_batch();
    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        double frequency = levelzero_io.read_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        double frequency_batch = levelzero_io.sample(batch_idx.at(sub_idx));
        EXPECT_DOUBLE_EQ(frequency, mock_freq.at(sub_idx) * 1e6);
        EXPECT_DOUBLE_EQ(frequency, frequency_batch);

        double throttle = levelzero_io.read_signal("LEVELZERO::GPU_CORE_THROTTLE_REASONS", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        double throttle_batch = levelzero_io.sample(batch_idx.at(sub_idx + m_num_gpu_subdevice));
        EXPECT_DOUBLE_EQ(throttle, mock_throttle.at(sub_idx));
        EXPECT_DOUBLE_EQ(throttle, throttle_batch);

        double energy_chip = levelzero_io.read_signal("LEVELZERO::GPU_CORE_ENERGY", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        double energy_chip_batch = levelzero_io.sample(batch_idx.at(sub_idx + 2 * m_num_gpu_subdevice));

        EXPECT_DOUBLE_EQ(energy_chip, mock_energy_chip.at(sub_idx)/1e6);
        EXPECT_DOUBLE_EQ(energy_chip, energy_chip_batch);
    }

    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        double energy = levelzero_io.read_signal("LEVELZERO::GPU_ENERGY", GEOPM_DOMAIN_GPU, gpu_idx);
        double energy_batch = levelzero_io.sample(batch_idx.at(3 * m_num_gpu_subdevice + gpu_idx));
        EXPECT_DOUBLE_EQ(energy, mock_energy.at(gpu_idx) / 1e6);
        EXPECT_DOUBLE_EQ(energy, energy_batch);
    }

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        double frequency_efficient = levelzero_io.read_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        double frequency_efficient_batch = levelzero_io.sample(batch_idx.at(sub_idx));
        EXPECT_DOUBLE_EQ(frequency_efficient, mock_freq.at(sub_idx) * 1e6);
        EXPECT_DOUBLE_EQ(frequency_efficient, frequency_efficient_batch);
    }
}

TEST_F(LevelZeroIOGroupTest, read_timestamp_batch_reverse)
{
    SetUpDefaultExpectCalls();
    std::vector<uint64_t> mock_energy = {630000000, 280000000, 470000000, 950000000};
    std::vector<uint64_t> mock_energy_timestamp = {153, 70, 300, 50};

    std::vector<uint64_t> mock_active_time = {123, 970, 550, 20, 52, 567, 888, 923};
    std::vector<uint64_t> mock_active_time_compute = {1, 90, 50, 0, 123, 144, 521, 445};
    std::vector<uint64_t> mock_active_time_copy = {12, 20, 30, 40, 44, 55, 66, 77};
    std::vector<uint64_t> mock_active_time_timestamp = {182, 970, 650, 33, 283, 331, 675, 9000};
    std::vector<uint64_t> mock_active_time_timestamp_compute = {12, 90, 150, 3, 772, 248, 932, 122};
    std::vector<uint64_t> mock_active_time_timestamp_copy = {50, 60, 53, 55, 66, 77, 88, 99};

    std::vector<int> energy_batch_idx;
    std::vector<int> active_time_batch_idx;
    std::vector<int> active_time_compute_batch_idx;
    std::vector<int> active_time_copy_batch_idx;

    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, nullptr);

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_active_time.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_active_time_compute.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_active_time_copy.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_active_time_timestamp.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_active_time_timestamp_compute.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_active_time_timestamp_copy.at(sub_idx)));

        active_time_batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_ACTIVE_TIME_TIMESTAMP", GEOPM_DOMAIN_GPU_CHIP, sub_idx));
        active_time_compute_batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_CORE_ACTIVE_TIME_TIMESTAMP", GEOPM_DOMAIN_GPU_CHIP, sub_idx));
        active_time_copy_batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_UNCORE_ACTIVE_TIME_TIMESTAMP", GEOPM_DOMAIN_GPU_CHIP, sub_idx));
    }

    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, energy_timestamp(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy_timestamp.at(gpu_idx)));

        energy_batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_ENERGY_TIMESTAMP", GEOPM_DOMAIN_GPU, gpu_idx));
    }

    levelzero_io.read_batch();
    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        double active_time = levelzero_io.sample(active_time_batch_idx.at(sub_idx)-1);
        double active_time_timestamp = levelzero_io.sample(active_time_batch_idx.at(sub_idx));

        EXPECT_DOUBLE_EQ(active_time, mock_active_time.at(sub_idx)/1e6);
        EXPECT_DOUBLE_EQ(active_time_timestamp, mock_active_time_timestamp.at(sub_idx)/1e6);

        double active_time_gpu = levelzero_io.sample(active_time_batch_idx.at(sub_idx)-1);
        double active_time_timestamp_gpu = levelzero_io.sample(active_time_batch_idx.at(sub_idx));

        EXPECT_DOUBLE_EQ(active_time_gpu, mock_active_time.at(sub_idx)/1e6);
        EXPECT_DOUBLE_EQ(active_time_timestamp_gpu, mock_active_time_timestamp.at(sub_idx)/1e6);

        double active_time_copy = levelzero_io.sample(active_time_batch_idx.at(sub_idx)-1);
        double active_time_timestamp_copy = levelzero_io.sample(active_time_batch_idx.at(sub_idx));

        EXPECT_DOUBLE_EQ(active_time_copy, mock_active_time.at(sub_idx)/1e6);
        EXPECT_DOUBLE_EQ(active_time_timestamp_copy, mock_active_time_timestamp.at(sub_idx)/1e6);
    }
    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        double energy = levelzero_io.sample(energy_batch_idx.at(gpu_idx)-1);
        double energy_timestamp = levelzero_io.sample(energy_batch_idx.at(gpu_idx));

        EXPECT_DOUBLE_EQ(energy, mock_energy.at(gpu_idx)/1e6);
        EXPECT_DOUBLE_EQ(energy_timestamp, mock_energy_timestamp.at(gpu_idx)/1e6);
    }
}


TEST_F(LevelZeroIOGroupTest, read_timestamp_batch)
{
    SetUpDefaultExpectCalls();
    std::vector<uint64_t> mock_energy = {630000000, 280000000, 470000000, 950000000};
    std::vector<uint64_t> mock_energy_timestamp = {153, 70, 300, 50};

    std::vector<uint64_t> mock_active_time = {123, 970, 550, 20, 52, 567, 888, 923};
    std::vector<uint64_t> mock_active_time_compute = {1, 90, 50, 0, 123, 144, 521, 445};
    std::vector<uint64_t> mock_active_time_copy = {12, 20, 30, 40, 44, 55, 66, 77};
    std::vector<uint64_t> mock_active_time_timestamp = {182, 970, 650, 33, 283, 331, 675, 9000};
    std::vector<uint64_t> mock_active_time_timestamp_compute = {12, 90, 150, 3, 772, 248, 932, 122};
    std::vector<uint64_t> mock_active_time_timestamp_copy = {50, 60, 53, 55, 66, 77, 88, 99};

    std::vector<int> energy_batch_idx;
    std::vector<int> active_time_batch_idx;
    std::vector<int> active_time_compute_batch_idx;
    std::vector<int> active_time_copy_batch_idx;

    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, nullptr);

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_active_time.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_active_time_compute.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_active_time_copy.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_active_time_timestamp.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_active_time_timestamp_compute.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_active_time_timestamp_copy.at(sub_idx)));

        active_time_batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_ACTIVE_TIME", GEOPM_DOMAIN_GPU_CHIP, sub_idx));
        active_time_compute_batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_CORE_ACTIVE_TIME", GEOPM_DOMAIN_GPU_CHIP, sub_idx));
        active_time_copy_batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_UNCORE_ACTIVE_TIME", GEOPM_DOMAIN_GPU_CHIP, sub_idx));
    }

    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, energy_timestamp(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy_timestamp.at(gpu_idx)));

        energy_batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::GPU_ENERGY", GEOPM_DOMAIN_GPU, gpu_idx));
    }

    levelzero_io.read_batch();
    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        double active_time = levelzero_io.sample(active_time_batch_idx.at(sub_idx));
        double active_time_timestamp = levelzero_io.sample(active_time_batch_idx.at(sub_idx)+1);

        EXPECT_DOUBLE_EQ(active_time, mock_active_time.at(sub_idx)/1e6);
        EXPECT_DOUBLE_EQ(active_time_timestamp, mock_active_time_timestamp.at(sub_idx)/1e6);

        double active_time_gpu = levelzero_io.sample(active_time_batch_idx.at(sub_idx));
        double active_time_timestamp_gpu = levelzero_io.sample(active_time_batch_idx.at(sub_idx)+1);

        EXPECT_DOUBLE_EQ(active_time_gpu, mock_active_time.at(sub_idx)/1e6);
        EXPECT_DOUBLE_EQ(active_time_timestamp_gpu, mock_active_time_timestamp.at(sub_idx)/1e6);

        double active_time_copy = levelzero_io.sample(active_time_batch_idx.at(sub_idx));
        double active_time_timestamp_copy = levelzero_io.sample(active_time_batch_idx.at(sub_idx)+1);

        EXPECT_DOUBLE_EQ(active_time_copy, mock_active_time.at(sub_idx)/1e6);
        EXPECT_DOUBLE_EQ(active_time_timestamp_copy, mock_active_time_timestamp.at(sub_idx)/1e6);
    }
    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        double energy = levelzero_io.sample(energy_batch_idx.at(gpu_idx));
        double energy_timestamp = levelzero_io.sample(energy_batch_idx.at(gpu_idx)+1);

        EXPECT_DOUBLE_EQ(energy, mock_energy.at(gpu_idx)/1e6);
        EXPECT_DOUBLE_EQ(energy_timestamp, mock_energy_timestamp.at(gpu_idx)/1e6);
    }
}

TEST_F(LevelZeroIOGroupTest, read_signal)
{
    SetUpDefaultExpectCalls();
    //Frequency
    std::vector<double> mock_freq_gpu = {1530, 1320, 420, 135, 900, 927, 293, 400};
    std::vector<double> mock_freq_mem = {130, 1020, 200, 150, 300, 442, 782, 1059};
    std::vector<double> mock_freq_min_gpu = {200, 320, 400, 350, 111, 222, 333, 444};
    std::vector<double> mock_freq_max_gpu = {2000, 3200, 4200, 1350, 555, 666, 777, 888};
    std::vector<double> mock_freq_min_mem = {100, 220, 300, 450, 999, 1010, 1111, 1212};
    std::vector<double> mock_freq_max_mem = {1000, 2200, 3200, 1450, 1313, 1414, 1515, 1616};
    //Active time
    std::vector<uint64_t> mock_active_time = {123, 970, 550, 20, 52, 567, 888, 923};
    std::vector<uint64_t> mock_active_time_compute = {1, 90, 50, 0, 123, 144, 521, 445};
    std::vector<uint64_t> mock_active_time_copy = {12, 20, 30, 40, 44, 55, 66, 77};
    //Power & energy
    std::vector<int32_t> mock_power_limit_min = {30000, 80000, 20000, 70000};
    std::vector<int32_t> mock_power_limit_max = {310000, 280000, 320000, 270000};
    std::vector<int32_t> mock_power_limit_tdp = {320000, 290000, 330000, 280000};
    std::vector<uint64_t> mock_energy = {630000000, 280000000, 470000000, 950000000};
    std::vector<uint64_t> mock_energy_chip = {330000000, 180000000, 270000000, 350000000,
                                              430000000, 280000000, 370000000, 450000000};

    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, nullptr);

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        //Frequency
        EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq_gpu.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_freq_mem.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_min(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq_min_gpu.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_max(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(mock_freq_max_gpu.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_min(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_freq_min_mem.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, frequency_max(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(mock_freq_max_mem.at(sub_idx)));

        //Power & energy
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy_chip.at(sub_idx)));

        //Active time
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL)).WillOnce(Return(mock_active_time.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillOnce(Return(mock_active_time_compute.at(sub_idx)));
        EXPECT_CALL(*m_device_pool, active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillOnce(Return(mock_active_time_copy.at(sub_idx)));
    }

    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        //Power & energy
        EXPECT_CALL(*m_device_pool, power_limit_min(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillOnce(Return(mock_power_limit_min.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_max(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillOnce(Return(mock_power_limit_max.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_tdp(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillOnce(Return(mock_power_limit_tdp.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, energy(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(mock_energy.at(gpu_idx)));
    }


    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        //Frequency
        double frequency = levelzero_io.read_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        double frequency_alias = levelzero_io.read_signal("GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, frequency_alias);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_gpu.at(sub_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::GPU_UNCORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_mem.at(sub_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::GPU_CORE_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_min_gpu.at(sub_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::GPU_CORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_max_gpu.at(sub_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::GPU_UNCORE_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_min_mem.at(sub_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::GPU_UNCORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_max_mem.at(sub_idx)*1e6);

        //Active time
        double active_time = levelzero_io.read_signal("LEVELZERO::GPU_ACTIVE_TIME", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(active_time, mock_active_time.at(sub_idx)/1e6);
        double active_time_compute = levelzero_io.read_signal("LEVELZERO::GPU_CORE_ACTIVE_TIME", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(active_time_compute, mock_active_time_compute.at(sub_idx)/1e6);
        double active_time_copy = levelzero_io.read_signal("LEVELZERO::GPU_UNCORE_ACTIVE_TIME", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(active_time_copy, mock_active_time_copy.at(sub_idx)/1e6);

        //Power & energy
        double energy = levelzero_io.read_signal("LEVELZERO::GPU_CORE_ENERGY", GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        EXPECT_DOUBLE_EQ(energy, mock_energy_chip.at(sub_idx)/1e6);
    }

    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        //Power & energy
        double power_lim = levelzero_io.read_signal("LEVELZERO::GPU_POWER_LIMIT_MIN_AVAIL", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_min.at(gpu_idx)/1e3);
        power_lim = levelzero_io.read_signal("LEVELZERO::GPU_POWER_LIMIT_MAX_AVAIL", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_max.at(gpu_idx)/1e3);
        power_lim = levelzero_io.read_signal("LEVELZERO::GPU_POWER_LIMIT_DEFAULT", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_tdp.at(gpu_idx)/1e3);

        double energy = levelzero_io.read_signal("LEVELZERO::GPU_ENERGY", GEOPM_DOMAIN_GPU, gpu_idx);
        double energy_alias = levelzero_io.read_signal("GPU_ENERGY", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(energy, energy_alias);
        EXPECT_DOUBLE_EQ(energy, mock_energy.at(gpu_idx)/1e6);
    }

    // Assume DerivativeSignals class functions as expected
    // Just check validity of derived signals
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::GPU_POWER"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::GPU_CORE_POWER"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::GPU_UTILIZATION"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::GPU_CORE_UTILIZATION"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::GPU_UNCORE_UTILIZATION"));
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
    SetUpDefaultExpectCalls();
    //Frequency
    std::vector<double> mock_freq_gpu = {1530, 1320, 420, 135, 900, 927, 293, 400};
    std::vector<double> mock_freq_mem = {130, 1020, 200, 150, 300, 442, 782, 1059};
    std::vector<double> mock_freq_min_gpu = {200, 320, 400, 350, 111, 222, 333, 444};
    std::vector<double> mock_freq_max_gpu = {2000, 3200, 4200, 1350, 555, 666, 777, 888};
    std::vector<double> mock_freq_min_mem = {100, 220, 300, 450, 999, 1010, 1111, 1212};
    std::vector<double> mock_freq_max_mem = {1000, 2200, 3200, 1450, 1313, 1414, 1515, 1616};
    //Active time
    std::vector<uint64_t> mock_active_time = {123, 970, 550, 20, 52, 567, 888, 923};
    std::vector<uint64_t> mock_active_time_compute = {1, 90, 50, 0, 123, 144, 521, 445};
    std::vector<uint64_t> mock_active_time_copy = {12, 20, 30, 40, 44, 55, 66, 77};
    //Power & energy
    std::vector<int32_t> mock_power_limit_min = {30000, 80000, 20000, 70000};
    std::vector<int32_t> mock_power_limit_max = {310000, 280000, 320000, 270000};
    std::vector<int32_t> mock_power_limit_tdp = {320000, 290000, 330000, 280000};
    std::vector<uint64_t> mock_energy = {630000000, 280000000, 470000000, 950000000};

    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, nullptr);

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.sample(0),
                               GEOPM_ERROR_INVALID, "batch_idx 0 out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_signal("LEVELZERO::INVALID", GEOPM_DOMAIN_GPU, 0),
                               GEOPM_ERROR_INVALID, "signal_name LEVELZERO::INVALID not valid for LevelZeroIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::INVALID", GEOPM_DOMAIN_GPU, 0),
                               GEOPM_ERROR_INVALID, "LEVELZERO::INVALID not valid for LevelZeroIOGroup");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_control("LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.adjust(0, 12345.6),
                               GEOPM_ERROR_INVALID, "batch_idx 0 out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.write_control("LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_type must be");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_control("LEVELZERO::INVALID", GEOPM_DOMAIN_GPU, 0),
                               GEOPM_ERROR_INVALID, "control_name LEVELZERO::INVALID not valid for LevelZeroIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.write_control("LEVELZERO::INVALID", GEOPM_DOMAIN_GPU, 0, 1530000000),
                               GEOPM_ERROR_INVALID, "LEVELZERO::INVALID not valid for LevelZeroIOGroup");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, m_num_gpu_subdevice),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, m_num_gpu_subdevice),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU_CHIP, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_control("LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_GPU_CHIP, m_num_gpu_subdevice),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_control("LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_GPU_CHIP, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.write_control("LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_GPU_CHIP, m_num_gpu_subdevice, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.write_control("LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_GPU_CHIP, -1, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::GPU_ACTIVE_TIME_TIMESTAMP", GEOPM_DOMAIN_GPU_CHIP, 0), GEOPM_ERROR_INVALID, "TIMESTAMP Signals are for batch use only.");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::GPU_UNCORE_ACTIVE_TIME_TIMESTAMP", GEOPM_DOMAIN_GPU_CHIP, 0), GEOPM_ERROR_INVALID, "TIMESTAMP Signals are for batch use only.");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::GPU_CORE_ACTIVE_TIME_TIMESTAMP", GEOPM_DOMAIN_GPU_CHIP, 0), GEOPM_ERROR_INVALID, "TIMESTAMP Signals are for batch use only.");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::GPU_ENERGY_TIMESTAMP", GEOPM_DOMAIN_GPU, 0), GEOPM_ERROR_INVALID, "TIMESTAMP Signals are for batch use only.");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::GPU_CORE_ENERGY_TIMESTAMP", GEOPM_DOMAIN_GPU_CHIP, 0), GEOPM_ERROR_INVALID, "TIMESTAMP Signals are for batch use only.");
}

TEST_F(LevelZeroIOGroupTest, signal_and_control_trimming)
{
    // The following was copy/pasted from SetUpDefaultExpect calls, with the lines commented out that will be
    // specifically examined by this test.
    //
    // Expectations for signal/control pruning code in the constructor
    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, // GPU_ACTIVE_TIME
                    active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_ACTIVE_TIME_TIMESTAMP
                    active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_ACTIVE_TIME
                    active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_ACTIVE_TIME_TIMESTAMP
                    active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_FREQUENCY_MAX_AVAIL
                    frequency_max(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        // GPU_CORE_FREQUENCY_MAX_CONTROL (signal pruning), GPU_CORE_FREQUENCY_MIN_CONTROL (signal pruning),
        // the save_control() call, GPU_CORE_FREQUENCY_MAX_CONTROL (control pruning) * 2,
        // and GPU_CORE_FREQUENCY_MIN_CONTROL (control pruning) * 2 = 7 times
        // EXPECT_CALL(*m_device_pool,
        //             frequency_range(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).Times(7);
        EXPECT_CALL(*m_device_pool, // GPU_CORE_FREQUENCY_MIN_AVAIL
                    frequency_min(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        // EXPECT_CALL(*m_device_pool, // GPU_CORE_FREQUENCY_STATUS
        //             frequency_status(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_FREQUENCY_EFFICIENT
                    frequency_efficient(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_THROTTLE_REASONS
                    frequency_throttle_reasons(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_CALL(*m_device_pool, // GPU_UNCORE_ACTIVE_TIME
                    active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));
        EXPECT_CALL(*m_device_pool, // GPU_UNCORE_ACTIVE_TIME_TIMESTAMP
                    active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));
        EXPECT_CALL(*m_device_pool, // GPU_UNCORE_FREQUENCY_MAX_AVAIL
                    frequency_max(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));
        EXPECT_CALL(*m_device_pool, // GPU_UNCORE_FREQUENCY_MIN_AVAIL
                    frequency_min(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_ENERGY
                    energy(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_CORE_ENERGY_TIMESTAMP
                    energy_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_ALL));
        // EXPECT_CALL(*m_device_pool, // GPU_UNCORE_FREQUENCY_STATUS
        //             frequency_status(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));
        // control pruning expectations
        // GPU_CORE_FREQUENCY_MAX_CONTROL, GPU_CORE_FREQUENCY_MIN_CONTROL, and the restore_control() direct call.
        // EXPECT_CALL(*m_device_pool,
        //             frequency_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE,
        //                               0, 0)).Times(3);

        EXPECT_CALL(*m_device_pool, // GPU_CORE_PERFORMANCE_FACTOR
                    performance_factor(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).Times(2);

        EXPECT_CALL(*m_device_pool, // GPU_CORE_PERFORMANCE_FACTOR_CONTROL
                    performance_factor_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE, 0));
    }

    // Expectations for signal pruning code in the constructor
    for (int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, // GPU_ENERGY
                    energy(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_ENERGY_TIMESTAMP
                    energy_timestamp(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_POWER_LIMIT_DEFAULT
                    power_limit_tdp(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_POWER_LIMIT_MAX_AVAIL
                    power_limit_max(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_CALL(*m_device_pool, // GPU_POWER_LIMIT_MIN_AVAIL
                    power_limit_min(GEOPM_DOMAIN_GPU, gpu_idx, MockLevelZero::M_DOMAIN_ALL));
    }
    // End copy/paste from SetUpDefualtExpectCalls

    // The implementation of the pruning code only tests each control on a
    // single domain index if a problem is encountered.  If there is a problem
    // on any chip, the signal is pruned and the remaining GPU_CHIPs are not
    // checked.
    //Frequency
    EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_GPU_CHIP, 0, MockLevelZero::M_DOMAIN_COMPUTE)).
                WillOnce(Throw(geopm::Exception("Not Supported", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__)));
    EXPECT_CALL(*m_device_pool, frequency_status(GEOPM_DOMAIN_GPU_CHIP, 0, MockLevelZero::M_DOMAIN_MEMORY)).
                WillRepeatedly(Throw(geopm::Exception("Invalid", GEOPM_ERROR_INVALID, __FILE__, __LINE__)));

    for (int sub_idx = 0; sub_idx < m_num_gpu_subdevice; ++sub_idx) {
        EXPECT_CALL(*m_device_pool, frequency_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE, _, _)).
                    WillRepeatedly(Throw(geopm::Exception("Not Supported", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__)));
        // frequency_range is called a non-standard number of times due to the implemetation of the pruning code.
        // Only one chip is checked if there is a failure.
        EXPECT_CALL(*m_device_pool,
                    frequency_range(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE)).Times(AtLeast(3));
    }

    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, nullptr);

    EXPECT_FALSE(levelzero_io.is_valid_signal("LEVELZERO::GPU_CORE_FREQUENCY_STATUS"));
    EXPECT_FALSE(levelzero_io.is_valid_signal("LEVELZERO::GPU_UNCORE_FREQUENCY_STATUS"));
    EXPECT_FALSE(levelzero_io.is_valid_control("LEVELZERO::GPU_CORE_FREQUENCY_MIN_CONTROL"));
    EXPECT_FALSE(levelzero_io.is_valid_control("LEVELZERO::GPU_CORE_FREQUENCY_MAX_CONTROL"));
}

TEST_F(LevelZeroIOGroupTest, save_restore_control)
{
    SetUpDefaultExpectCalls();
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool, m_mock_save_ctl);

    // Verify that all controls can be read as signals
    auto control_set = levelzero_io.control_names();
    auto signal_set = levelzero_io.signal_names();
    std::vector<std::string> difference(control_set.size());

    auto it = std::set_difference(control_set.cbegin(), control_set.cend(),
                                  signal_set.cbegin(), signal_set.cend(),
                                  difference.begin());
    difference.resize(it - difference.begin());

    std::string err_msg = "The following controls are not readable as signals: \n";
    for (auto &sig : difference) {
        err_msg += "    " + sig + '\n';
    }
    EXPECT_EQ((unsigned int) 0, difference.size()) << err_msg;

    std::string file_name = "tmp_file";
    EXPECT_CALL(*m_mock_save_ctl, write_json(file_name));
    levelzero_io.save_control(file_name);
    EXPECT_CALL(*m_mock_save_ctl, restore(_));
    levelzero_io.restore_control(file_name);
}

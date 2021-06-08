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

        std::shared_ptr<MockLevelZeroDevicePool> m_device_pool;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
};

void LevelZeroIOGroupTest::SetUp()
{
    const int num_board = 1;
    const int num_package = 2;
    const int num_board_accelerator = 4;
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

    EXPECT_CALL(*m_device_pool, num_accelerator()).WillRepeatedly(Return(num_board_accelerator));
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
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);
    std::map<int, double> batch_value;
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<double> mock_standby_control = {1, 0, 1, 0};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        batch_value[(levelzero_io.push_control("LEVELZERO::FREQUENCY_GPU_CONTROL",
                                        GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx))] = mock_freq.at(accel_idx)*1e6;
        batch_value[(levelzero_io.push_control("FREQUENCY_ACCELERATOR_CONTROL",
                                        GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx))] = mock_freq.at(accel_idx)*1e6;
        EXPECT_CALL(*m_device_pool,
                    frequency_control_gpu(accel_idx, mock_freq.at(accel_idx))).Times(2);

        batch_value[(levelzero_io.push_control("LEVELZERO::STANDBY_MODE_CONTROL",
                                        GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx))] = mock_standby_control.at(accel_idx);
        EXPECT_CALL(*m_device_pool, standby_mode_control(accel_idx,
                                                         mock_standby_control.at(accel_idx))).Times(1);
    }

    for (auto& sv: batch_value) {
        // Given that we are mocking LEVELZERODevicePool the actual setting here doesn't matter
        EXPECT_NO_THROW(levelzero_io.adjust(sv.first, sv.second));
    }
    EXPECT_NO_THROW(levelzero_io.write_batch());
}

TEST_F(LevelZeroIOGroupTest, write_control)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);
    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<double> mock_standby_control = {1, 0, 1, 0};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool,
                    frequency_control_gpu(accel_idx, mock_freq.at(accel_idx))).Times(2);

        EXPECT_NO_THROW(levelzero_io.write_control("LEVELZERO::FREQUENCY_GPU_CONTROL",
                                              GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx,
                                              mock_freq.at(accel_idx)*1e6));

        EXPECT_NO_THROW(levelzero_io.write_control("FREQUENCY_ACCELERATOR_CONTROL",
                                              GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx,
                                              mock_freq.at(accel_idx)*1e6));

        EXPECT_CALL(*m_device_pool, standby_mode_control(accel_idx,
                                                         mock_standby_control.at(accel_idx))).Times(1);

        EXPECT_NO_THROW(levelzero_io.write_control("LEVELZERO::STANDBY_MODE_CONTROL",
                                              GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx,
                                              mock_standby_control.at(accel_idx)));
    }
}

TEST_F(LevelZeroIOGroupTest, read_signal_and_batch)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<int> batch_idx;

    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool);

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_gpu(accel_idx)).WillRepeatedly(Return(mock_freq.at(accel_idx)));
    }
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        batch_idx.push_back(levelzero_io.push_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx));
    }
    levelzero_io.read_batch();
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        double frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        double frequency_batch = levelzero_io.sample(batch_idx.at(accel_idx));

        EXPECT_DOUBLE_EQ(frequency, mock_freq.at(accel_idx)*1e6);
        EXPECT_DOUBLE_EQ(frequency, frequency_batch);
    }

    mock_freq = {1630, 1420, 520, 235};
    //second round of testing with a modified value
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_gpu(accel_idx)).WillRepeatedly(Return(mock_freq.at(accel_idx)));
    }
    levelzero_io.read_batch();
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        double frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        double frequency_batch = levelzero_io.sample(batch_idx.at(accel_idx));

        EXPECT_DOUBLE_EQ(frequency, (mock_freq.at(accel_idx))*1e6);
        EXPECT_DOUBLE_EQ(frequency, frequency_batch);
    }
}

TEST_F(LevelZeroIOGroupTest, read_signal)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);

    //Frequency
    std::vector<double> mock_freq_gpu = {1530, 1320, 420, 135};
    std::vector<double> mock_freq_mem = {130, 1020, 200, 150};
    std::vector<double> mock_freq_min_gpu = {200, 320, 400, 350};
    std::vector<double> mock_freq_max_gpu = {2000, 3200, 4200, 1350};
    std::vector<double> mock_freq_min_mem = {100, 220, 300, 450};
    std::vector<double> mock_freq_max_mem = {1000, 2200, 3200, 1450};
    std::vector<double> mock_freq_range_min_gpu = {1200, 620, 500, 530};
    std::vector<double> mock_freq_range_max_gpu = {3120, 900, 5200, 3500};
    std::vector<double> mock_freq_throttle_time = {200000, 600020, 100500, 101530};
    std::vector<double> mock_freq_throttle_time_timestamp = {314520, 901000, 58200, 303500};
    std::vector<double> mock_freq_throttle_reason = {3, 0, 0xFF, 0xA};
    //Active time
    std::vector<uint64_t> mock_active_time = {123, 970, 550, 20};
    std::vector<uint64_t> mock_active_time_timestamp = {182, 970, 650, 33};
    std::vector<uint64_t> mock_active_time_compute = {1, 90, 50, 0};
    std::vector<uint64_t> mock_active_time_timestamp_compute = {12, 90, 150, 3};
    std::vector<uint64_t> mock_active_time_copy = {12, 20, 30, 40};
    std::vector<uint64_t> mock_active_time_timestamp_copy = {50, 60, 53, 55};
    std::vector<uint64_t> mock_active_time_media_decode = {21, 24, 35, 46};
    std::vector<uint64_t> mock_active_time_timestamp_media_decode = {51, 62, 55, 53};
    //Temperature
    std::vector<double> mock_temperature = {45, 60, 68, 92};
    std::vector<double> mock_temperature_gpu = {50, 65, 78, 99};
    std::vector<double> mock_temperature_mem = {4, 63, 60, 90};
    //Power & energy
    std::vector<int32_t> mock_power_limit_min = {30000, 80000, 20000, 70000};
    std::vector<int32_t> mock_power_limit_max = {310000, 280000, 320000, 270000};
    std::vector<int32_t> mock_power_limit_tdp = {320000, 290000, 330000, 280000};
    std::vector<int32_t> mock_power_limit_interval_sustained = {30000, 26000, 34000, 70000};
    std::vector<int32_t> mock_power_limit_sustained = {310000, 280000, 320000, 270000};
    std::vector<bool> mock_power_limit_enabled_sustained = {0, 1, 1, 1};
    std::vector<int32_t> mock_power_limit_burst = {300000, 270000, 300000, 250000};
    std::vector<bool> mock_power_limit_enabled_burst = {1, 1, 0, 1};
    std::vector<int32_t> mock_power_limit_peak_ac = {100000, 730000, 600000, 450000};
    std::vector<uint64_t> mock_energy = {630000000, 280000000, 470000000, 950000000};
    std::vector<uint64_t> mock_energy_timestamp = {153, 70, 300, 50};

    std::vector<double> mock_performance_factor = {0, 100, 50, 72};
    std::vector<double> mock_standby_mode = {0, 1, 0, 1};
    std::vector<double> mock_memory_allocated = {0, 1, 0.5, 0.21};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {

        //Frequency
        EXPECT_CALL(*m_device_pool, frequency_status_gpu(accel_idx)).WillRepeatedly(Return(mock_freq_gpu.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, frequency_status_mem(accel_idx)).WillRepeatedly(Return(mock_freq_mem.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, frequency_min_gpu(accel_idx)).WillRepeatedly(Return(mock_freq_min_gpu.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, frequency_max_gpu(accel_idx)).WillRepeatedly(Return(mock_freq_max_gpu.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, frequency_min_mem(accel_idx)).WillRepeatedly(Return(mock_freq_min_mem.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, frequency_max_mem(accel_idx)).WillRepeatedly(Return(mock_freq_max_mem.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, frequency_range_min_gpu(accel_idx)).WillRepeatedly(Return(mock_freq_range_min_gpu.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, frequency_range_max_gpu(accel_idx)).WillRepeatedly(Return(mock_freq_range_max_gpu.at(accel_idx)));

        EXPECT_CALL(*m_device_pool, frequency_throttle_time_gpu(accel_idx)).WillRepeatedly(Return(mock_freq_throttle_time.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, frequency_throttle_time_timestamp_gpu(accel_idx)).WillRepeatedly(Return(mock_freq_throttle_time_timestamp.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, frequency_status_throttle_reason_gpu(accel_idx)).WillRepeatedly(Return(mock_freq_throttle_reason.at(accel_idx)));

        //Active time
        EXPECT_CALL(*m_device_pool, active_time(accel_idx)).WillRepeatedly(Return(mock_active_time.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp(accel_idx)).WillRepeatedly(Return(mock_active_time_timestamp.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, active_time_compute(accel_idx)).WillRepeatedly(Return(mock_active_time_compute.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp_compute(accel_idx)).WillRepeatedly(Return(mock_active_time_timestamp_compute.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, active_time_copy(accel_idx)).WillRepeatedly(Return(mock_active_time_copy.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp_copy(accel_idx)).WillRepeatedly(Return(mock_active_time_timestamp_copy.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, active_time_media_decode(accel_idx)).WillRepeatedly(Return(mock_active_time_media_decode.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, active_time_timestamp_media_decode(accel_idx)).WillRepeatedly(Return(mock_active_time_timestamp_media_decode.at(accel_idx)));

        //Temperature
        EXPECT_CALL(*m_device_pool, temperature(accel_idx)).WillRepeatedly(Return(mock_temperature.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, temperature_gpu(accel_idx)).WillRepeatedly(Return(mock_temperature_gpu.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, temperature_memory(accel_idx)).WillRepeatedly(Return(mock_temperature_mem.at(accel_idx)));

        //Power & energy
        EXPECT_CALL(*m_device_pool, power_limit_min(accel_idx)).WillRepeatedly(Return(mock_power_limit_min.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_max(accel_idx)).WillRepeatedly(Return(mock_power_limit_max.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_tdp(accel_idx)).WillRepeatedly(Return(mock_power_limit_tdp.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_enabled_sustained(accel_idx)).WillRepeatedly(Return(mock_power_limit_enabled_sustained.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_sustained(accel_idx)).WillRepeatedly(Return(mock_power_limit_sustained.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_interval_sustained(accel_idx)).WillRepeatedly(Return(mock_power_limit_interval_sustained.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_enabled_burst(accel_idx)).WillRepeatedly(Return(mock_power_limit_enabled_burst.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_burst(accel_idx)).WillRepeatedly(Return(mock_power_limit_burst.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power_limit_peak_ac(accel_idx)).WillRepeatedly(Return(mock_power_limit_peak_ac.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, energy(accel_idx)).WillRepeatedly(Return(mock_energy.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, energy_timestamp(accel_idx)).WillRepeatedly(Return(mock_energy_timestamp.at(accel_idx)));

        //Misc
        EXPECT_CALL(*m_device_pool, performance_factor(accel_idx)).WillRepeatedly(Return(mock_performance_factor.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, standby_mode(accel_idx)).WillRepeatedly(Return(mock_standby_mode.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, memory_allocated(accel_idx)).WillRepeatedly(Return(mock_memory_allocated.at(accel_idx)));
    }

    LevelZeroIOGroup levelzero_io(*m_platform_topo, *m_device_pool);

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        //Frequency
        double frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        double frequency_alias = levelzero_io.read_signal("FREQUENCY_ACCELERATOR", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, frequency_alias);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_gpu.at(accel_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_MEMORY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_mem.at(accel_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_MIN_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_min_gpu.at(accel_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_MAX_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_max_gpu.at(accel_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_MIN_MEMORY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_min_mem.at(accel_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_MAX_MEMORY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_max_mem.at(accel_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_RANGE_MIN_GPU_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_range_min_gpu.at(accel_idx)*1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::FREQUENCY_RANGE_MAX_GPU_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_range_max_gpu.at(accel_idx)*1e6);

        frequency = levelzero_io.read_signal("LEVELZERO::THROTTLE_TIME_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_throttle_time.at(accel_idx)/1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::THROTTLE_TIME_TIMESTAMP_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_throttle_time_timestamp.at(accel_idx)/1e6);
        frequency = levelzero_io.read_signal("LEVELZERO::THROTTLE_REASONS_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq_throttle_reason.at(accel_idx));


        //Active time
        double active_time = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(active_time, mock_active_time.at(accel_idx)/1e6);
        double active_time_timestamp = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_TIMESTAMP", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(active_time_timestamp, mock_active_time_timestamp.at(accel_idx)/1e6);
        double active_time_compute = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_COMPUTE", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(active_time_compute, mock_active_time_compute.at(accel_idx)/1e6);
        double active_time_timestamp_compute = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_TIMESTAMP_COMPUTE", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(active_time_timestamp_compute, mock_active_time_timestamp_compute.at(accel_idx)/1e6);
        double active_time_copy = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_COPY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(active_time_copy, mock_active_time_copy.at(accel_idx)/1e6);
        double active_time_timestamp_copy = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_TIMESTAMP_COPY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(active_time_timestamp_copy, mock_active_time_timestamp_copy.at(accel_idx)/1e6);
        double active_time_media_decode = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_MEDIA_DECODE", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(active_time_media_decode, mock_active_time_media_decode.at(accel_idx)/1e6);
        double active_time_timestamp_media_decode = levelzero_io.read_signal("LEVELZERO::ACTIVE_TIME_TIMESTAMP_MEDIA_DECODE", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(active_time_timestamp_media_decode, mock_active_time_timestamp_media_decode.at(accel_idx)/1e6);

        //Temperature
        double temperature = levelzero_io.read_signal("LEVELZERO::TEMPERATURE", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(temperature, mock_temperature.at(accel_idx));
        temperature = levelzero_io.read_signal("LEVELZERO::TEMPERATURE_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(temperature, mock_temperature_gpu.at(accel_idx));
        temperature = levelzero_io.read_signal("LEVELZERO::TEMPERATURE_MEMORY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(temperature, mock_temperature_mem.at(accel_idx));

        //Power & energy
        double power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_MIN", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_min.at(accel_idx)/1e3);
        power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_MAX", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_max.at(accel_idx)/1e3);
        power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_DEFAULT", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_tdp.at(accel_idx)/1e3);
        power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_ENABLED_BURST", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_enabled_burst.at(accel_idx));
        power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_ENABLED_SUSTAINED", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_enabled_sustained.at(accel_idx));
        power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_INTERVAL_SUSTAINED", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_interval_sustained.at(accel_idx)/1e3);
        power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_BURST", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_burst.at(accel_idx)/1e3);
        power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_SUSTAINED", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_sustained.at(accel_idx)/1e3);
        power_lim = levelzero_io.read_signal("LEVELZERO::POWER_LIMIT_PEAK_AC", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power_lim, mock_power_limit_peak_ac.at(accel_idx)/1e3);

        double energy = levelzero_io.read_signal("LEVELZERO::ENERGY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(energy, mock_energy.at(accel_idx)/1e6);
        double energy_timestamp = levelzero_io.read_signal("LEVELZERO::ENERGY_TIMESTAMP", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(energy_timestamp, mock_energy_timestamp.at(accel_idx)/1e6);

        //Misc
        double misc = levelzero_io.read_signal("LEVELZERO::PERFORMANCE_FACTOR", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(misc, mock_performance_factor.at(accel_idx));
        misc = levelzero_io.read_signal("LEVELZERO::STANDBY_MODE", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(misc, mock_standby_mode.at(accel_idx));
        misc = levelzero_io.read_signal("LEVELZERO::MEMORY_ALLOCATED", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(misc, mock_memory_allocated.at(accel_idx));
    }

    // Assume DerivativeSignals class functions as expected
    // Just check validity of derived signals
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::POWER"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::UTILIZATION"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::UTILIZATION_COMPUTE"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::UTILIZATION_COPY"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::UTILIZATION_MEDIA_DECODE"));
    ASSERT_TRUE(levelzero_io.is_valid_signal("LEVELZERO::THROTTLE_RATIO_GPU"));

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

    std::vector<int> batch_idx;

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_gpu(accel_idx)).WillRepeatedly(Return(mock_freq.at(accel_idx)));
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

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, num_accelerator),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, num_accelerator),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.read_signal("LEVELZERO::FREQUENCY_GPU", GEOPM_DOMAIN_BOARD_ACCELERATOR, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_control("LEVELZERO::FREQUENCY_GPU_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR, num_accelerator),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.push_control("LEVELZERO::FREQUENCY_GPU_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.write_control("LEVELZERO::FREQUENCY_GPU_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR, num_accelerator, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(levelzero_io.write_control("LEVELZERO::FREQUENCY_GPU_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR, -1, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
}

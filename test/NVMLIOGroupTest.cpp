/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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
#include "NVMLIOGroup.hpp"
#include "geopm_test.hpp"
#include "MockNVMLDevicePool.hpp"
#include "MockPlatformTopo.hpp"

using geopm::NVMLIOGroup;
using geopm::PlatformTopo;
using geopm::Exception;
using testing::Return;

class NVMLIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void write_affinitization(const std::string &affinitization_str);

        std::shared_ptr<MockNVMLDevicePool> m_device_pool;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
};

void NVMLIOGroupTest::SetUp()
{
    const int num_board = 1;
    const int num_package = 2;
    const int num_board_accelerator = 4;
    const int num_core = 20;
    const int num_cpu = 40;

    m_device_pool = std::make_shared<MockNVMLDevicePool>();
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

void NVMLIOGroupTest::TearDown()
{
}

TEST_F(NVMLIOGroupTest, push_control_adjust_write_batch)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);
    std::map<int, double> batch_value;
    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<double> mock_power = {153600, 70000, 300000, 50000};
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        batch_value[(nvml_io.push_control("NVML::FREQUENCY_CONTROL",
                                        GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx))] = mock_freq.at(accel_idx)*1e6;
        batch_value[(nvml_io.push_control("FREQUENCY_ACCELERATOR_CONTROL",
                                        GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx))] = mock_freq.at(accel_idx)*1e6;
        EXPECT_CALL(*m_device_pool,
                    frequency_control_sm(accel_idx, mock_freq.at(accel_idx),
                                         mock_freq.at(accel_idx))).Times(2);

        batch_value[(nvml_io.push_control("NVML::FREQUENCY_RESET_CONTROL",
                                        GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx))] = mock_freq.at(accel_idx);
        EXPECT_CALL(*m_device_pool, frequency_reset_control(accel_idx)).Times(1);

        batch_value[(nvml_io.push_control("NVML::POWER_LIMIT_CONTROL",
                                        GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx))] = mock_power.at(accel_idx)/1e3;
        batch_value[(nvml_io.push_control("POWER_ACCELERATOR_LIMIT_CONTROL",
                                        GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx))] = mock_power.at(accel_idx)/1e3;
        EXPECT_CALL(*m_device_pool, power_control(accel_idx, mock_power.at(accel_idx))).Times(2);
    }

    for (auto& sv: batch_value) {

        // Given that we are mocking NVMLDevicePool the actual setting here doesn't matter
        EXPECT_NO_THROW(nvml_io.adjust(sv.first, sv.second));
    }
    EXPECT_NO_THROW(nvml_io.write_batch());
}

TEST_F(NVMLIOGroupTest, write_control)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);
    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<double> mock_power = {153600, 70000, 300000, 50000};
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool,
                    frequency_control_sm(accel_idx, mock_freq.at(accel_idx),
                                         mock_freq.at(accel_idx))).Times(2);
        EXPECT_NO_THROW(nvml_io.write_control("NVML::FREQUENCY_CONTROL",
                                              GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx,
                                              mock_freq.at(accel_idx)*1e6));
        EXPECT_NO_THROW(nvml_io.write_control("FREQUENCY_ACCELERATOR_CONTROL",
                                              GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx,
                                              mock_freq.at(accel_idx)*1e6));

        EXPECT_CALL(*m_device_pool, frequency_reset_control(accel_idx)).Times(1);
        EXPECT_NO_THROW(nvml_io.write_control("NVML::FREQUENCY_RESET_CONTROL",
                                              GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx, 12345));

        EXPECT_CALL(*m_device_pool, power_control(accel_idx, mock_power.at(accel_idx))).Times(2);
        EXPECT_NO_THROW(nvml_io.write_control("NVML::POWER_LIMIT_CONTROL",
                                              GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx,
                                              mock_power.at(accel_idx)/1e3));
        EXPECT_NO_THROW(nvml_io.write_control("POWER_ACCELERATOR_LIMIT_CONTROL",
                                              GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx,
                                              mock_power.at(accel_idx)/1e3));
    }
}

TEST_F(NVMLIOGroupTest, read_signal_and_batch)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<int> batch_idx;

    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool);

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_sm(accel_idx)).WillRepeatedly(Return(mock_freq.at(accel_idx)));
    }
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        batch_idx.push_back(nvml_io.push_signal("NVML::FREQUENCY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx));
    }
    nvml_io.read_batch();
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        double frequency = nvml_io.read_signal("NVML::FREQUENCY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        double frequency_batch = nvml_io.sample(batch_idx.at(accel_idx));

        EXPECT_DOUBLE_EQ(frequency, mock_freq.at(accel_idx)*1e6);
        EXPECT_DOUBLE_EQ(frequency, frequency_batch);
    }

    mock_freq = {1630, 1420, 520, 235};
    //second round of testing with a modified value
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_sm(accel_idx)).WillRepeatedly(Return(mock_freq.at(accel_idx)));
    }
    nvml_io.read_batch();
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        double frequency = nvml_io.read_signal("NVML::FREQUENCY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        double frequency_batch = nvml_io.sample(batch_idx.at(accel_idx));

        EXPECT_DOUBLE_EQ(frequency, (mock_freq.at(accel_idx))*1e6);
        EXPECT_DOUBLE_EQ(frequency, frequency_batch);
    }
}

TEST_F(NVMLIOGroupTest, read_signal)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);
    const int num_cpu = m_platform_topo->num_domain(GEOPM_DOMAIN_CPU);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<double> mock_utilization_accelerator = {100, 90, 50, 0};
    std::vector<double> mock_power = {153600, 70000, 300000, 50000};
    std::vector<double> mock_power_limit = {300000, 270000, 300000, 250000};
    std::vector<double> mock_freq_mem = {877, 877, 877, 877};
    std::vector<double> mock_throttle_reasons = {0, 1, 3, 128};
    std::vector<double> mock_temperature = {45, 60, 68, 92};
    std::vector<double> mock_energy = {630000, 280000, 470000, 950000};
    std::vector<double> mock_performance_state = {0, 2, 3, 5};
    std::vector<double> mock_pcie_rx_throughput = {4000, 3000, 2000, 0};
    std::vector<double> mock_pcie_tx_throughput = {2000, 3000, 4000, 100};
    std::vector<double> mock_utilization_mem = {25, 50, 100, 75};

    std::vector<int> active_process_list = {40961, 40962, 40963};

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_sm(accel_idx)).WillRepeatedly(Return(mock_freq.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, utilization(accel_idx)).WillRepeatedly(Return(mock_utilization_accelerator.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, power(accel_idx)).WillRepeatedly(Return(mock_power.at(accel_idx)));;
        EXPECT_CALL(*m_device_pool, power_limit(accel_idx)).WillRepeatedly(Return(mock_power_limit.at(accel_idx)));;
        EXPECT_CALL(*m_device_pool, frequency_status_mem(accel_idx)).WillRepeatedly(Return(mock_freq_mem.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, throttle_reasons(accel_idx)).WillRepeatedly(Return(mock_throttle_reasons.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, temperature(accel_idx)).WillRepeatedly(Return(mock_temperature.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, energy(accel_idx)).WillRepeatedly(Return(mock_energy.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, performance_state(accel_idx)).WillRepeatedly(Return(mock_performance_state.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, throughput_rx_pcie(accel_idx)).WillRepeatedly(Return(mock_pcie_rx_throughput.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, throughput_tx_pcie(accel_idx)).WillRepeatedly(Return(mock_pcie_tx_throughput.at(accel_idx)));
        EXPECT_CALL(*m_device_pool, utilization_mem(accel_idx)).WillRepeatedly(Return(mock_utilization_mem.at(accel_idx)));

    }

    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        EXPECT_CALL(*m_device_pool, active_process_list(cpu_idx)).WillRepeatedly(Return(active_process_list));;
    }

    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool);

    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        double frequency = nvml_io.read_signal("NVML::FREQUENCY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        double frequency_alias = nvml_io.read_signal("FREQUENCY_ACCELERATOR", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency, frequency_alias);
        EXPECT_DOUBLE_EQ(frequency, mock_freq.at(accel_idx)*1e6);

        double utilization_accelerator = nvml_io.read_signal("NVML::UTILIZATION_ACCELERATOR", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(utilization_accelerator, mock_utilization_accelerator.at(accel_idx)/100);

        double throttle_reasons = nvml_io.read_signal("NVML::THROTTLE_REASONS", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(throttle_reasons, mock_throttle_reasons.at(accel_idx));

        double power = nvml_io.read_signal("NVML::POWER", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        double power_alias = nvml_io.read_signal("POWER_ACCELERATOR", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(power, power_alias);
        EXPECT_DOUBLE_EQ(power, mock_power.at(accel_idx)/1e3);

        double frequency_mem = nvml_io.read_signal("NVML::FREQUENCY_MEMORY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(frequency_mem, mock_freq_mem.at(accel_idx)*1e6);

        double temperature = nvml_io.read_signal("NVML::TEMPERATURE", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(temperature, mock_temperature.at(accel_idx));

        double total_energy_consumption = nvml_io.read_signal("NVML::TOTAL_ENERGY_CONSUMPTION", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(total_energy_consumption, mock_energy.at(accel_idx)/1e3);

        double performance_state = nvml_io.read_signal("NVML::PERFORMANCE_STATE", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(performance_state, mock_performance_state.at(accel_idx));

        double pcie_rx_throughput = nvml_io.read_signal("NVML::PCIE_RX_THROUGHPUT", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(pcie_rx_throughput, mock_pcie_rx_throughput.at(accel_idx)*1024);

        double pcie_tx_throughput = nvml_io.read_signal("NVML::PCIE_TX_THROUGHPUT", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(pcie_tx_throughput, mock_pcie_tx_throughput.at(accel_idx)*1024);

        double utilization_mem = nvml_io.read_signal("NVML::UTILIZATION_MEMORY", GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
        EXPECT_DOUBLE_EQ(utilization_mem, mock_utilization_mem.at(accel_idx)/100);
    }

    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        // FIXME: The most complex signal is the cpu accelerator active afifinitzation signal, which is currently
        //        not fully testable due to needing a running process for get affinity.  For now using a no throw check
        EXPECT_NO_THROW(nvml_io.read_signal("NVML::CPU_ACCELERATOR_ACTIVE_AFFINITIZATION", GEOPM_DOMAIN_CPU, cpu_idx));
    }
}

//Test case: Error path testing including:
//              - Attempt to push a signal at an invalid domain level
//              - Attempt to push an invalid signal
//              - Attempt to sample without a read_batch prior
//              - Attempt to read a signal at an invalid domain level
//              - Attempt to push a control at an invalid domain level
//              - Attempt to adjust a non-existent batch index
//              - Attempt to write a control at an invalid domain level
TEST_F(NVMLIOGroupTest, error_path)
{
    const int num_accelerator = m_platform_topo->num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);

    std::vector<int> batch_idx;

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    for (int accel_idx = 0; accel_idx < num_accelerator; ++accel_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_sm(accel_idx)).WillRepeatedly(Return(mock_freq.at(accel_idx)));
    }
    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool);

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_signal("NVML::FREQUENCY", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.sample(0),
                               GEOPM_ERROR_INVALID, "batch_idx 0 out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.read_signal("NVML::FREQUENCY", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_signal("NVML::INVALID", GEOPM_DOMAIN_BOARD_ACCELERATOR, 0),
                               GEOPM_ERROR_INVALID, "signal_name NVML::INVALID not valid for NVMLIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.read_signal("NVML::INVALID", GEOPM_DOMAIN_BOARD_ACCELERATOR, 0),
                               GEOPM_ERROR_INVALID, "NVML::INVALID not valid for NVMLIOGroup");

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_control("NVML::FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.adjust(0, 12345.6),
                               GEOPM_ERROR_INVALID, "batch_idx 0 out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.write_control("NVML::FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD, 0, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_type must be");

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_control("NVML::INVALID", GEOPM_DOMAIN_BOARD_ACCELERATOR, 0),
                               GEOPM_ERROR_INVALID, "control_name NVML::INVALID not valid for NVMLIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.write_control("NVML::INVALID", GEOPM_DOMAIN_BOARD_ACCELERATOR, 0, 1530000000),
                               GEOPM_ERROR_INVALID, "NVML::INVALID not valid for NVMLIOGroup");

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_signal("NVML::FREQUENCY", GEOPM_DOMAIN_BOARD_ACCELERATOR, num_accelerator),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_signal("NVML::FREQUENCY", GEOPM_DOMAIN_BOARD_ACCELERATOR, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.read_signal("NVML::FREQUENCY", GEOPM_DOMAIN_BOARD_ACCELERATOR, num_accelerator),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.read_signal("NVML::FREQUENCY", GEOPM_DOMAIN_BOARD_ACCELERATOR, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_control("NVML::FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR, num_accelerator),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_control("NVML::FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.write_control("NVML::FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR, num_accelerator, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.write_control("NVML::FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD_ACCELERATOR, -1, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
}

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
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PluginFactory.hpp"
#include "NVMLIOGroup.hpp"
#include "geopm_test.hpp"
#include "MockNVMLDevicePool.hpp"
#include "MockPlatformTopo.hpp"
#include "MockSaveControl.hpp"

using geopm::NVMLIOGroup;
using geopm::PlatformTopo;
using geopm::Exception;
using testing::Return;
using testing::_;
using testing::AtLeast;

class NVMLIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void write_affinitization(const std::string &affinitization_str);

        std::shared_ptr<MockNVMLDevicePool> m_device_pool;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
        std::shared_ptr<MockSaveControl> m_mock_save_ctl;

        const std::string M_PLUGIN_NAME = "NVML";
        const std::string M_NAME_PREFIX = M_PLUGIN_NAME + "::";
};

void NVMLIOGroupTest::SetUp()
{
    const int num_board = 1;
    const int num_package = 2;
    const int num_gpu = 4;
    const int num_core = 20;
    const int num_cpu = 40;

    m_device_pool = std::make_shared<MockNVMLDevicePool>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();
    m_mock_save_ctl = std::make_shared<MockSaveControl>();

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

    EXPECT_CALL(*m_device_pool, num_gpu()).WillRepeatedly(Return(num_gpu));

    std::vector<unsigned int> mock_supported_freq = {135, 142, 407, 414, 760, 882, 1170, 1530};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, frequency_supported_sm(gpu_idx)).WillRepeatedly(Return(mock_supported_freq));
    }
}

void NVMLIOGroupTest::TearDown()
{
}

TEST_F(NVMLIOGroupTest, valid_signals)
{
    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool, nullptr);
    for (const auto &sig : nvml_io.signal_names()) {
        EXPECT_TRUE(nvml_io.is_valid_signal(sig));
        EXPECT_NE(GEOPM_DOMAIN_INVALID, nvml_io.signal_domain_type(sig));
        EXPECT_LT(-1, nvml_io.signal_behavior(sig));
    }
}

TEST_F(NVMLIOGroupTest, push_control_adjust_write_batch)
{
    const int num_gpu = m_platform_topo->num_domain(GEOPM_DOMAIN_GPU);
    std::map<int, double> batch_value;
    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool, nullptr);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<double> mock_power = {153600, 70000, 300000, 50000};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        batch_value[(nvml_io.push_control(M_NAME_PREFIX + "GPU_FREQUENCY_CONTROL",
                                        GEOPM_DOMAIN_GPU, gpu_idx))] = mock_freq.at(gpu_idx) * 1e6;
        batch_value[(nvml_io.push_control("GPU_FREQUENCY_CONTROL",
                                        GEOPM_DOMAIN_GPU, gpu_idx))] = mock_freq.at(gpu_idx) * 1e6;
        EXPECT_CALL(*m_device_pool,
                    frequency_control_sm(gpu_idx, mock_freq.at(gpu_idx),
                                         mock_freq.at(gpu_idx))).Times(2);

        batch_value[(nvml_io.push_control(M_NAME_PREFIX + "GPU_FREQUENCY_RESET_CONTROL",
                                        GEOPM_DOMAIN_GPU, gpu_idx))] = mock_freq.at(gpu_idx);
        EXPECT_CALL(*m_device_pool, frequency_reset_control(gpu_idx)).Times(1);

        batch_value[(nvml_io.push_control(M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL",
                                        GEOPM_DOMAIN_GPU, gpu_idx))] = mock_power.at(gpu_idx) / 1e3;
        batch_value[(nvml_io.push_control("GPU_POWER_LIMIT_CONTROL",
                                        GEOPM_DOMAIN_GPU, gpu_idx))] = mock_power.at(gpu_idx) / 1e3;
        EXPECT_CALL(*m_device_pool, power_control(gpu_idx, mock_power.at(gpu_idx))).Times(2);
    }

    for (auto& sv: batch_value) {

        // Given that we are mocking NVMLDevicePool the actual setting here doesn't matter
        EXPECT_NO_THROW(nvml_io.adjust(sv.first, sv.second));
    }
    EXPECT_NO_THROW(nvml_io.write_batch());
}

TEST_F(NVMLIOGroupTest, write_control)
{
    const int num_gpu = m_platform_topo->num_domain(GEOPM_DOMAIN_GPU);
    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool, nullptr);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<double> mock_power = {153600, 70000, 300000, 50000};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool,
                    frequency_control_sm(gpu_idx, mock_freq.at(gpu_idx),
                                         mock_freq.at(gpu_idx))).Times(2);
        EXPECT_NO_THROW(nvml_io.write_control(M_NAME_PREFIX + "GPU_FREQUENCY_CONTROL",
                                              GEOPM_DOMAIN_GPU, gpu_idx,
                                              mock_freq.at(gpu_idx) * 1e6));
        // Verify the write was cached properly
        double frequency = nvml_io.read_signal(M_NAME_PREFIX + "GPU_FREQUENCY_CONTROL",
                                               GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(frequency, mock_freq.at(gpu_idx) * 1e6);

        EXPECT_NO_THROW(nvml_io.write_control("GPU_FREQUENCY_CONTROL",
                                              GEOPM_DOMAIN_GPU, gpu_idx,
                                              mock_freq.at(gpu_idx) * 1e6));

        EXPECT_CALL(*m_device_pool, frequency_reset_control(gpu_idx)).Times(1);
        EXPECT_NO_THROW(nvml_io.write_control(M_NAME_PREFIX + "GPU_FREQUENCY_RESET_CONTROL",
                                              GEOPM_DOMAIN_GPU, gpu_idx, 12345));

        EXPECT_CALL(*m_device_pool, power_control(gpu_idx, mock_power.at(gpu_idx))).Times(2);
        EXPECT_NO_THROW(nvml_io.write_control(M_NAME_PREFIX + "GPU_POWER_LIMIT_CONTROL",
                                              GEOPM_DOMAIN_GPU, gpu_idx,
                                              mock_power.at(gpu_idx) / 1e3));
        EXPECT_NO_THROW(nvml_io.write_control("GPU_POWER_LIMIT_CONTROL",
                                              GEOPM_DOMAIN_GPU, gpu_idx,
                                              mock_power.at(gpu_idx) / 1e3));
    }
}

TEST_F(NVMLIOGroupTest, read_signal_and_batch)
{
    const int num_gpu = m_platform_topo->num_domain(GEOPM_DOMAIN_GPU);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<int> batch_idx;

    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool, nullptr);

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_sm(gpu_idx)).WillRepeatedly(Return(mock_freq.at(gpu_idx)));
    }
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        batch_idx.push_back(nvml_io.push_signal(M_NAME_PREFIX + "GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU, gpu_idx));
    }
    nvml_io.read_batch();
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        double frequency = nvml_io.read_signal(M_NAME_PREFIX + "GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU, gpu_idx);
        double frequency_batch = nvml_io.sample(batch_idx.at(gpu_idx));

        EXPECT_DOUBLE_EQ(frequency, mock_freq.at(gpu_idx) * 1e6);
        EXPECT_DOUBLE_EQ(frequency, frequency_batch);
    }

    mock_freq = {1630, 1420, 520, 235};
    //second round of testing with a modified value
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_sm(gpu_idx)).WillRepeatedly(Return(mock_freq.at(gpu_idx)));
    }
    nvml_io.read_batch();
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        double frequency = nvml_io.read_signal(M_NAME_PREFIX + "GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU, gpu_idx);
        double frequency_batch = nvml_io.sample(batch_idx.at(gpu_idx));

        EXPECT_DOUBLE_EQ(frequency, (mock_freq.at(gpu_idx)) * 1e6);
        EXPECT_DOUBLE_EQ(frequency, frequency_batch);
    }
}

TEST_F(NVMLIOGroupTest, read_signal)
{
    const int num_gpu = m_platform_topo->num_domain(GEOPM_DOMAIN_GPU);
    const int num_cpu = m_platform_topo->num_domain(GEOPM_DOMAIN_CPU);

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    std::vector<unsigned int> mock_supported_freq = {135, 142, 407, 414, 760, 882, 1170, 1530};
    std::vector<double> mock_utilization_gpu = {100, 90, 50, 0};
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

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_sm(gpu_idx)).WillRepeatedly(Return(mock_freq.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, frequency_supported_sm(gpu_idx)).WillRepeatedly(Return(mock_supported_freq));
        EXPECT_CALL(*m_device_pool, utilization(gpu_idx)).WillRepeatedly(Return(mock_utilization_gpu.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, power(gpu_idx)).WillRepeatedly(Return(mock_power.at(gpu_idx)));;
        EXPECT_CALL(*m_device_pool, power_limit(gpu_idx)).WillRepeatedly(Return(mock_power_limit.at(gpu_idx)));;
        EXPECT_CALL(*m_device_pool, frequency_status_mem(gpu_idx)).WillRepeatedly(Return(mock_freq_mem.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, throttle_reasons(gpu_idx)).WillRepeatedly(Return(mock_throttle_reasons.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, temperature(gpu_idx)).WillRepeatedly(Return(mock_temperature.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, energy(gpu_idx)).WillRepeatedly(Return(mock_energy.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, performance_state(gpu_idx)).WillRepeatedly(Return(mock_performance_state.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, throughput_rx_pcie(gpu_idx)).WillRepeatedly(Return(mock_pcie_rx_throughput.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, throughput_tx_pcie(gpu_idx)).WillRepeatedly(Return(mock_pcie_tx_throughput.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, utilization_mem(gpu_idx)).WillRepeatedly(Return(mock_utilization_mem.at(gpu_idx)));
    }

    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        EXPECT_CALL(*m_device_pool, active_process_list(cpu_idx)).WillRepeatedly(Return(active_process_list));;
    }

    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool, nullptr);
    std::sort(mock_supported_freq.begin(), mock_supported_freq.end());

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        double frequency = nvml_io.read_signal(M_NAME_PREFIX + "GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU, gpu_idx);
        double frequency_alias = nvml_io.read_signal("GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(frequency, frequency_alias);
        EXPECT_DOUBLE_EQ(frequency, mock_freq.at(gpu_idx) * 1e6);

        double frequency_min = nvml_io.read_signal(M_NAME_PREFIX + "GPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_GPU, gpu_idx);
        double frequency_min_alias = nvml_io.read_signal("GPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(frequency_min, mock_supported_freq.front() * 1e6);
        EXPECT_DOUBLE_EQ(frequency_min, frequency_min_alias);

        double frequency_max = nvml_io.read_signal(M_NAME_PREFIX + "GPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_GPU, gpu_idx);
        double frequency_max_alias = nvml_io.read_signal("GPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(frequency_max, mock_supported_freq.back() * 1e6);
        EXPECT_DOUBLE_EQ(frequency_max, frequency_max_alias);

        double utilization_gpu = nvml_io.read_signal(M_NAME_PREFIX + "GPU_UTILIZATION", GEOPM_DOMAIN_GPU, gpu_idx);
        double utilization_gpu_alias = nvml_io.read_signal("GPU_UTILIZATION", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(utilization_gpu, mock_utilization_gpu.at(gpu_idx) / 100);
        EXPECT_DOUBLE_EQ(utilization_gpu, utilization_gpu_alias);

        double throttle_reasons = nvml_io.read_signal(M_NAME_PREFIX + "GPU_THROTTLE_REASONS", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(throttle_reasons, mock_throttle_reasons.at(gpu_idx));

        double power = nvml_io.read_signal(M_NAME_PREFIX + "GPU_POWER", GEOPM_DOMAIN_GPU, gpu_idx);
        double power_alias = nvml_io.read_signal("GPU_POWER", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(power, power_alias);
        EXPECT_DOUBLE_EQ(power, mock_power.at(gpu_idx) / 1e3);

        double frequency_mem = nvml_io.read_signal(M_NAME_PREFIX + "GPU_MEMORY_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(frequency_mem, mock_freq_mem.at(gpu_idx) * 1e6);

        double temperature = nvml_io.read_signal(M_NAME_PREFIX + "GPU_TEMPERATURE", GEOPM_DOMAIN_GPU, gpu_idx);
        double temperature_alias = nvml_io.read_signal("GPU_TEMPERATURE", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(temperature, mock_temperature.at(gpu_idx));
        EXPECT_DOUBLE_EQ(temperature, temperature_alias);

        double total_energy_consumption = nvml_io.read_signal(M_NAME_PREFIX + "GPU_ENERGY_CONSUMPTION_TOTAL", GEOPM_DOMAIN_GPU, gpu_idx);
        double total_energy_consumption_alias = nvml_io.read_signal("GPU_ENERGY", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(total_energy_consumption, mock_energy.at(gpu_idx) / 1e3);
        EXPECT_DOUBLE_EQ(total_energy_consumption, total_energy_consumption_alias);

        double performance_state = nvml_io.read_signal(M_NAME_PREFIX + "GPU_PERFORMANCE_STATE", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(performance_state, mock_performance_state.at(gpu_idx));

        double pcie_rx_throughput = nvml_io.read_signal(M_NAME_PREFIX + "GPU_PCIE_RX_THROUGHPUT", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(pcie_rx_throughput, mock_pcie_rx_throughput.at(gpu_idx) * 1024);

        double pcie_tx_throughput = nvml_io.read_signal(M_NAME_PREFIX + "GPU_PCIE_TX_THROUGHPUT", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(pcie_tx_throughput, mock_pcie_tx_throughput.at(gpu_idx) * 1024);

        double utilization_mem = nvml_io.read_signal(M_NAME_PREFIX + "GPU_MEMORY_UTILIZATION", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(utilization_mem, mock_utilization_mem.at(gpu_idx) / 100);

        frequency = nvml_io.read_signal(M_NAME_PREFIX + "GPU_FREQUENCY_CONTROL",
                                        GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(frequency, 0); // 0 until first write
    }

    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        // FIXME: The most complex signal is the cpu gpu active afifinitzation signal, which is currently
        //        not fully testable due to needing a running process for get affinity.  For now using a no throw check
        double affin = NAN;
        EXPECT_NO_THROW(affin = nvml_io.read_signal(M_NAME_PREFIX + "GPU_CPU_ACTIVE_AFFINITIZATION", GEOPM_DOMAIN_CPU, cpu_idx));
        EXPECT_DOUBLE_EQ(affin, -1);
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
    const int num_gpu = m_platform_topo->num_domain(GEOPM_DOMAIN_GPU);

    std::vector<int> batch_idx;

    std::vector<double> mock_freq = {1530, 1320, 420, 135};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, frequency_status_sm(gpu_idx)).WillRepeatedly(Return(mock_freq.at(gpu_idx)));
    }

    std::vector<unsigned int> mock_supported_freq = {};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, frequency_supported_sm(gpu_idx)).WillRepeatedly(Return(mock_supported_freq));
    }
    GEOPM_EXPECT_THROW_MESSAGE(NVMLIOGroup nvml_io_fail(*m_platform_topo, *m_device_pool, nullptr),
                               GEOPM_ERROR_INVALID,
                               "No supported frequencies found for gpu");

    mock_supported_freq = {135, 142, 407, 414, 760, 882, 1170, 1530};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, frequency_supported_sm(gpu_idx)).WillRepeatedly(Return(mock_supported_freq));
    }

    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool, nullptr);

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_signal(M_NAME_PREFIX + "GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.sample(0),
                               GEOPM_ERROR_INVALID, "batch_idx 0 out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.read_signal(M_NAME_PREFIX + "GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_signal(M_NAME_PREFIX + "INVALID", GEOPM_DOMAIN_GPU, 0),
                               GEOPM_ERROR_INVALID, "signal_name NVML::INVALID not valid for NVMLIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.read_signal(M_NAME_PREFIX + "INVALID", GEOPM_DOMAIN_GPU, 0),
                               GEOPM_ERROR_INVALID, M_NAME_PREFIX + "INVALID not valid for NVMLIOGroup");

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_control(M_NAME_PREFIX + "GPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.adjust(0, 12345.6),
                               GEOPM_ERROR_INVALID, "batch_idx 0 out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.write_control(M_NAME_PREFIX + "GPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD, 0, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_type must be");

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_control(M_NAME_PREFIX + "INVALID", GEOPM_DOMAIN_GPU, 0),
                               GEOPM_ERROR_INVALID, "control_name NVML::INVALID not valid for NVMLIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.write_control(M_NAME_PREFIX + "INVALID", GEOPM_DOMAIN_GPU, 0, 1530000000),
                               GEOPM_ERROR_INVALID, M_NAME_PREFIX + "INVALID not valid for NVMLIOGroup");

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_signal(M_NAME_PREFIX + "GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU, num_gpu),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_signal(M_NAME_PREFIX + "GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.read_signal(M_NAME_PREFIX + "GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU, num_gpu),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.read_signal(M_NAME_PREFIX + "GPU_FREQUENCY_STATUS", GEOPM_DOMAIN_GPU, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_control(M_NAME_PREFIX + "GPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_GPU, num_gpu),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.push_control(M_NAME_PREFIX + "GPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_GPU, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.write_control(M_NAME_PREFIX + "GPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_GPU, num_gpu, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(nvml_io.write_control(M_NAME_PREFIX + "GPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_GPU, -1, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
}

TEST_F(NVMLIOGroupTest, save_restore_control)
{
    NVMLIOGroup nvml_io(*m_platform_topo, *m_device_pool, m_mock_save_ctl);

    const int num_gpu = m_platform_topo->num_domain(GEOPM_DOMAIN_GPU);
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, power_limit(gpu_idx)).WillRepeatedly(Return(123));;
    }

    std::string file_name = "tmp_file";
    EXPECT_CALL(*m_mock_save_ctl, write_json(file_name));
    nvml_io.save_control(file_name);
    EXPECT_CALL(*m_mock_save_ctl, restore(_));
    nvml_io.restore_control(file_name);
}

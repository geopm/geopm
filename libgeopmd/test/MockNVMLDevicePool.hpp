/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKNVMLDEVICEPOOL_HPP_INCLUDE
#define MOCKNVMLDEVICEPOOL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "NVMLDevicePool.hpp"

class MockNVMLDevicePool : public geopm::NVMLDevicePool
{
    public:
        MOCK_METHOD(int, num_gpu, (), (const, override));
        MOCK_METHOD((std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >),
                    cpu_affinity_ideal_mask, (int), (const, override));
        MOCK_METHOD(uint64_t, frequency_status_sm, (int), (const, override));
        MOCK_METHOD(std::vector<unsigned int>, frequency_supported_sm, (int), (const, override));
        MOCK_METHOD(uint64_t, utilization, (int), (const, override));
        MOCK_METHOD(uint64_t, power, (int), (const, override));
        MOCK_METHOD(uint64_t, power_limit, (int), (const, override));
        MOCK_METHOD(uint64_t, frequency_status_mem, (int), (const, override));
        MOCK_METHOD(uint64_t, throttle_reasons, (int), (const, override));
        MOCK_METHOD(uint64_t, temperature, (int), (const, override));
        MOCK_METHOD(uint64_t, energy, (int), (const, override));
        MOCK_METHOD(uint64_t, performance_state, (int), (const, override));
        MOCK_METHOD(uint64_t, throughput_rx_pcie, (int), (const, override));
        MOCK_METHOD(uint64_t, throughput_tx_pcie, (int), (const, override));
        MOCK_METHOD(uint64_t, utilization_mem, (int), (const, override));
        MOCK_METHOD(std::vector<int>, active_process_list, (int), (const, override));
        MOCK_METHOD(void, frequency_control_sm, (int, int, int), (const, override));
        MOCK_METHOD(void, frequency_reset_control, (int), (const, override));
        MOCK_METHOD(void, power_control, (int, int), (const, override));
        MOCK_METHOD(bool, is_privileged_access, (), (const, override));
        MOCK_METHOD(void, reset, (), (override));
};

#endif

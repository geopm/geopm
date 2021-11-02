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

#ifndef MOCKNVMLDEVICEPOOL_HPP_INCLUDE
#define MOCKNVMLDEVICEPOOL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "NVMLDevicePool.hpp"

class MockNVMLDevicePool : public geopm::NVMLDevicePool
{
    public:
        MOCK_METHOD(int, num_accelerator, (), (const, override));
        MOCK_METHOD(cpu_set_t *, cpu_affinity_ideal_mask, (int), (const, override));
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
};

#endif

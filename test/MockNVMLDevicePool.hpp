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

#ifndef MOCKNVMLDEVICEPOOL_HPP_INCLUDE
#define MOCKNVMLDEVICEPOOL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "NVMLDevicePool.hpp"

class MockNVMLDevicePool : public geopm::NVMLDevicePool
{
    public:
        MOCK_CONST_METHOD0(num_accelerator,
                           int(void));
        MOCK_CONST_METHOD1(cpu_affinity_ideal_mask,
                           cpu_set_t *(int));
        MOCK_CONST_METHOD1(frequency_status,
                           uint64_t(int));
        MOCK_CONST_METHOD1(frequency_status_sm,
                           uint64_t(int));
        MOCK_CONST_METHOD1(utilization,
                           uint64_t(int));
        MOCK_CONST_METHOD1(power,
                           uint64_t(int));
        MOCK_CONST_METHOD1(power_limit,
                           uint64_t(int));
        MOCK_CONST_METHOD1(frequency_status_mem,
                           uint64_t(int));
        MOCK_CONST_METHOD1(throttle_reasons,
                           uint64_t(int));
        MOCK_CONST_METHOD1(temperature,
                           uint64_t(int));
        MOCK_CONST_METHOD1(energy,
                           uint64_t(int));
        MOCK_CONST_METHOD1(performance_state,
                           uint64_t(int));
        MOCK_CONST_METHOD1(throughput_rx_pcie,
                           uint64_t(int));
        MOCK_CONST_METHOD1(throughput_tx_pcie,
                           uint64_t(int));
        MOCK_CONST_METHOD1(utilization_mem,
                           uint64_t(int));
        MOCK_CONST_METHOD1(active_process_list,
                           std::vector<int>(int));
        MOCK_CONST_METHOD3(frequency_control_sm,
                           void(int, int, int));
        MOCK_CONST_METHOD1(frequency_reset_control,
                           void(int));
        MOCK_CONST_METHOD2(power_control,
                           void(int, int));
};

#endif

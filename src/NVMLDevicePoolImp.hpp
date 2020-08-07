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

#ifndef NVMLDEVICEPOOLIMP_HPP_INCLUDE
#define NVMLDEVICEPOOLIMP_HPP_INCLUDE

#include "NVMLDevicePool.hpp"

namespace geopm
{

    class NVMLDevicePoolImp : public NVMLDevicePool
    {
        public:
            NVMLDevicePoolImp(const int num_cpu);
            virtual ~NVMLDevicePoolImp();
            virtual int num_accelerator(void) const override;
            virtual cpu_set_t *cpu_affinity_ideal_mask(int accel_idx) const override;
            virtual uint64_t frequency_status_sm(int accel_idx) const override;
            virtual uint64_t utilization(int accel_idx) const override;
            virtual uint64_t power(int accel_idx) const override;
            virtual uint64_t power_limit(int accel_idx) const override;
            virtual uint64_t frequency_status_mem(int accel_idx) const override;
            virtual uint64_t throttle_reasons(int accel_idx) const override;
            virtual uint64_t temperature(int accel_idx) const override;
            virtual uint64_t energy(int accel_idx) const override;
            virtual uint64_t performance_state(int accel_idx) const override;
            virtual uint64_t throughput_rx_pcie(int accel_idx) const override;
            virtual uint64_t throughput_tx_pcie(int accel_idx) const override;
            virtual uint64_t utilization_mem(int accel_idx) const override;
            virtual std::vector<int> active_process_list(int accel_idx) const override;

            virtual void frequency_control_sm(int accel_idx, int min_freq, int max_freq) const override;
            virtual void frequency_reset_control(int accel_idx) const override;
            virtual void power_control(int accel_idx, int setting) const override;

        private:
            const unsigned int M_MAX_CONTEXTS;
            const unsigned int M_NUM_CPU;
            virtual void check_accel_range(int accel_idx) const;
            virtual void check_nvml_result(nvmlReturn_t nvml_result, int error, std::string message, int line) const;
            unsigned int m_num_accelerator;
            std::vector<nvmlDevice_t> m_nvml_device;
    };
}
#endif

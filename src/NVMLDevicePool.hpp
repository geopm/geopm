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

#ifndef NVMLDEVICEPOOL_HPP_INCLUDE
#define NVMLDEVICEPOOL_HPP_INCLUDE

#include <vector>
#include <string>
#include <cstdint>

#include <nvml.h>

#include "geopm_sched.h"

namespace geopm
{

    class NVMLDevicePool
    {
        public:
            NVMLDevicePool() = default;
            virtual ~NVMLDevicePool() = default;
            /// @brief Number of accelerators on the platform.
            virtual int num_accelerator(void) const = 0;
            /// @brief CPU Affinitization mask for a particular accelerator
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual cpu_set_t *ideal_cpu_affinitization_mask(int accel_idx) const = 0;
            /// @brief Get the NVML device streaming multiprocessor frequency
            //         in MHz
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual uint64_t frequency_status_sm(int accel_idx) const = 0;
            /// @brief Get the NVML device utilization metric
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual double utilization(int accel_idx) const = 0;
            /// @brief Get the NVML device power in milliwatts
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual uint64_t power (int accel_idx) const = 0;
            /// @brief Get the NVML device memory subsystem frequency in MHz
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual uint64_t frequency_status_mem(int accel_idx) const = 0;
            /// @brief Get the current NVML device clock throttle reasons
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual uint64_t throttle_reasons(int accel_idx) const = 0;
            /// @brief Get the current NVML device temperature
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual uint64_t temperature(int accel_idx) const = 0;
            /// @brief Get the total energy consumed counter value for
            //         an NVML device in millijoules
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual uint64_t energy(int accel_idx) const = 0;
            /// @brief Get the current performance state of an NVML
            //         device
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual uint64_t performance_state(int accel_idx) const = 0;
            /// @brief Get the pcie rx throughput over a 20ms period for
            //         an NVML device
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual uint64_t throughput_rx_pcie(int accel_idx) const = 0;
            /// @brief Get the pcie tx throughput over a 20ms period for
            //         an NVML device
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual uint64_t throughput_tx_pcie(int accel_idx) const = 0;
            /// @brief Get the NVML device memory Utilization metric
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual double utilization_mem(int accel_idx) const = 0;
            /// @brief Get the list of PIDs actively running on an NVML
            //         device
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual std::vector<int> active_process_list(int accel_idx) const = 0;

            /// @brief Set min and max frequency for NVML device
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            /// @param [in] min_freq Target min frequency in MHz
            /// @param [in] max_freq Target max frequency in MHz
            virtual void frequency_control_sm(int accel_idx, int min_freq, int max_freq) const = 0;
            /// @brief Reset min and max frequency for NVML device
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            virtual void frequency_reset_control(int accel_idx) const = 0;
            /// @brief Set power limit for NVML device
            /// @param [in] accel_idx The index indicating a particular
            ///        accelerator
            /// @param [in] setting Power cap in milliwatts
            virtual void power_control(int accel_idx, int setting) const = 0;
        private:
    };

    const NVMLDevicePool &nvml_device_pool(int num_cpu);
}
#endif

/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NVMLDEVICEPOOL_HPP_INCLUDE
#define NVMLDEVICEPOOL_HPP_INCLUDE

#include <vector>
#include <string>
#include <cstdint>

#include "geopm_sched.h"

namespace geopm
{

    class NVMLDevicePool
    {
        public:
            NVMLDevicePool() = default;
            virtual ~NVMLDevicePool() = default;
            /// @brief Number of GPUs on the platform.
            /// @return Number of NVML GPUs.
            virtual int num_gpu(void) const = 0;
            /// @brief CPU Affinitization mask for a particular GPU
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return CPU to GPU idea affinitization bitmask.
            virtual cpu_set_t *cpu_affinity_ideal_mask(int gpu_idx) const = 0;
            /// @brief Get the NVML device streaming multiprocessor frequency
            ///        in MHz.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU streaming multiproccesor frequency in MHz.
            virtual uint64_t frequency_status_sm(int gpu_idx) const = 0;
            /// @brief Get the supported NVML device streaming multiprocessor frequencies
            ///        in MHz.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU supported streaming multiproccesor frequencies in MHz.
            virtual std::vector<unsigned int> frequency_supported_sm(int gpu_idx) const = 0;
            /// @brief Get the NVML device utilization metric.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU streaming multiprocessor utilization
            ///         percentage as a whole number from 0 to 100.
            virtual uint64_t utilization(int gpu_idx) const = 0;
            /// @brief Get the NVML device power in milliwatts.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU power consumption in milliwatts.
            virtual uint64_t power (int gpu_idx) const = 0;
            /// @brief Get the NVML device power limit in milliwatts.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU power limit in milliwatts.
            virtual uint64_t power_limit (int gpu_idx) const = 0;
            /// @brief Get the NVML device memory subsystem frequency in MHz.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU memory frequency in MHz.
            virtual uint64_t frequency_status_mem(int gpu_idx) const = 0;
            /// @brief Get the current NVML device clock throttle reasons.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU clock throttle reasons as defined
            ///         in nvml.h.
            virtual uint64_t throttle_reasons(int gpu_idx) const = 0;
            /// @brief Get the current NVML device temperature.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU temperature in Celsius.
            virtual uint64_t temperature(int gpu_idx) const = 0;
            /// @brief Get the total energy consumed counter value for
            ///        an NVML device in millijoules.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU energy consumption in millijoules.
            virtual uint64_t energy(int gpu_idx) const = 0;
            /// @brief Get the current performance state of an NVML
            ///        device.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU performance state, defined by the
            ///         NVML API as 0 to 15, with 0 being maximum performance,
            ///         15 being minimum performance, and 32 being unknown.
            virtual uint64_t performance_state(int gpu_idx) const = 0;
            /// @brief Get the pcie receive throughput over a 20ms period for
            ///        an NVML device.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU pcie receive throughput in kilobytes per second.
            virtual uint64_t throughput_rx_pcie(int gpu_idx) const = 0;
            /// @brief Get the pcie transmit throughput over a 20ms period for
            ///        an NVML device.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU pcie transmit throughput in kilobytes per second.
            virtual uint64_t throughput_tx_pcie(int gpu_idx) const = 0;
            /// @brief Get the NVML device memory Utilization metric
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return GPU memory utilization percentage
            ///         as a whole number from 0 to 100.
            virtual uint64_t utilization_mem(int gpu_idx) const = 0;
            /// @brief Get the list of PIDs with an active context on an NVML
            ///        device.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @return List of PIDs that are associated with a context on the
            ///         specified NVML device.
            virtual std::vector<int> active_process_list(int gpu_idx) const = 0;

            /// @brief Set min and max frequency for NVML device.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @param [in] min_freq Target min frequency in MHz.
            /// @param [in] max_freq Target max frequency in MHz.
            virtual void frequency_control_sm(int gpu_idx, int min_freq, int max_freq) const = 0;
            /// @brief Reset min and max frequency for NVML device.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            virtual void frequency_reset_control(int gpu_idx) const = 0;
            /// @brief Set power limit for NVML device.
            /// @param [in] gpu_idx The index indicating a particular
            ///        GPU.
            /// @param [in] setting Power cap in milliwatts.
            virtual void power_control(int gpu_idx, int setting) const = 0;

            virtual bool is_privileged_access(void) const = 0;
        private:
    };

    const NVMLDevicePool &nvml_device_pool(int num_cpu);
}
#endif

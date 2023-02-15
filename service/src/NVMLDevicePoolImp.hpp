/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
            virtual int num_gpu(void) const override;
            virtual cpu_set_t *cpu_affinity_ideal_mask(int gpu_idx) const override;
            virtual uint64_t frequency_status_sm(int gpu_idx) const override;
            virtual std::vector<unsigned int> frequency_supported_sm(int gpu_idx) const override;
            virtual uint64_t utilization(int gpu_idx) const override;
            virtual uint64_t power(int gpu_idx) const override;
            virtual uint64_t power_limit(int gpu_idx) const override;
            virtual uint64_t frequency_status_mem(int gpu_idx) const override;
            virtual uint64_t throttle_reasons(int gpu_idx) const override;
            virtual uint64_t temperature(int gpu_idx) const override;
            virtual uint64_t energy(int gpu_idx) const override;
            virtual uint64_t performance_state(int gpu_idx) const override;
            virtual uint64_t throughput_rx_pcie(int gpu_idx) const override;
            virtual uint64_t throughput_tx_pcie(int gpu_idx) const override;
            virtual uint64_t utilization_mem(int gpu_idx) const override;
            virtual std::vector<int> active_process_list(int gpu_idx) const override;

            virtual void frequency_control_sm(int gpu_idx, int min_freq, int max_freq) const override;
            virtual void frequency_reset_control(int gpu_idx) const override;
            virtual void power_control(int gpu_idx, int setting) const override;
            virtual bool is_privileged_access(void) const override;

        private:
            const unsigned int M_MAX_CONTEXTS;
            const unsigned int M_MAX_FREQUENCIES;
            const unsigned int M_NUM_CPU;
            virtual void check_gpu_range(int gpu_idx) const;
            virtual void check_nvml_result(nvmlReturn_t nvml_result, int error, const std::string &message, int line) const;
            unsigned int m_num_gpu;
            std::vector<nvmlDevice_t> m_nvml_device;
    };
}
#endif

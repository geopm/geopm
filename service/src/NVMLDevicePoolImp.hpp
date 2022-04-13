/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
            virtual int num_accelerator(void) const override;
            virtual cpu_set_t *cpu_affinity_ideal_mask(int accel_idx) const override;
            virtual uint64_t frequency_status_sm(int accel_idx) const override;
            virtual std::vector<unsigned int> frequency_supported_sm(int accel_idx) const override;
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
            const unsigned int M_MAX_FREQUENCIES;
            const unsigned int M_NUM_CPU;
            virtual void check_accel_range(int accel_idx) const;
            virtual void check_nvml_result(nvmlReturn_t nvml_result, int error, const std::string &message, int line) const;
            unsigned int m_num_accelerator;
            std::vector<nvmlDevice_t> m_nvml_device;
    };
}
#endif

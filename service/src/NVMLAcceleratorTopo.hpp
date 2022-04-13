/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NVMLACCELERATORTOPO_HPP_INCLUDE
#define NVMLACCELERATORTOPO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <set>

#include "AcceleratorTopo.hpp"

namespace geopm
{
    class NVMLDevicePool;

    class NVMLAcceleratorTopo : public AcceleratorTopo
    {
        public:
            NVMLAcceleratorTopo();
            NVMLAcceleratorTopo(const NVMLDevicePool &device_pool, const int num_cpu);
            virtual ~NVMLAcceleratorTopo() = default;
            int num_accelerator(void) const override;
            int num_accelerator(int domain_type) const override;
            std::set<int> cpu_affinity_ideal(int accel_idx) const override;
            std::set<int> cpu_affinity_ideal(int domain_type, int accel_idx) const override;
        private:
            const NVMLDevicePool &m_nvml_device_pool;
            std::vector<std::set<int> > m_cpu_affinity_ideal;
    };
}
#endif

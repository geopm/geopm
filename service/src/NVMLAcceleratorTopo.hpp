/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NVMLGPUTOPO_HPP_INCLUDE
#define NVMLGPUTOPO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <set>

#include "GPUTopo.hpp"

namespace geopm
{
    class NVMLDevicePool;

    class NVMLGPUTopo : public GPUTopo
    {
        public:
            NVMLGPUTopo();
            NVMLGPUTopo(const NVMLDevicePool &device_pool, const int num_cpu);
            virtual ~NVMLGPUTopo() = default;
            int num_gpu(void) const override;
            int num_gpu(int domain_type) const override;
            std::set<int> cpu_affinity_ideal(int gpu_idx) const override;
            std::set<int> cpu_affinity_ideal(int domain_type, int gpu_idx) const override;
        private:
            const NVMLDevicePool &m_nvml_device_pool;
            std::vector<std::set<int> > m_cpu_affinity_ideal;
    };
}
#endif

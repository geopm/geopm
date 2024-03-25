/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEVELZEROGPUTOPO_HPP_INCLUDE
#define LEVELZEROGPUTOPO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <set>

#include "GPUTopo.hpp"

namespace geopm
{
    class LevelZeroDevicePool;

    class LevelZeroGPUTopo : public GPUTopo
    {
        public:
            LevelZeroGPUTopo();
            LevelZeroGPUTopo(const LevelZeroDevicePool &device_pool,
                                     const int num_cpu);
            virtual ~LevelZeroGPUTopo() = default;
            int num_gpu(void) const override;
            int num_gpu(int domain) const override;
            std::set<int> cpu_affinity_ideal(int gpu_idx) const override;
            std::set<int> cpu_affinity_ideal(int domain, int gpu_idx) const override;
        private:
            const LevelZeroDevicePool &m_levelzero_device_pool;
            std::vector<std::set<int> > m_cpu_affinity_ideal;
            std::vector<std::set<int> > m_cpu_affinity_ideal_chip;
    };
}
#endif

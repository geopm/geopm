/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEVELZEROACCELERATORTOPO_HPP_INCLUDE
#define LEVELZEROACCELERATORTOPO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <set>

#include "AcceleratorTopo.hpp"

namespace geopm
{
    class LevelZeroDevicePool;

    class LevelZeroAcceleratorTopo : public AcceleratorTopo
    {
        public:
            LevelZeroAcceleratorTopo();
            LevelZeroAcceleratorTopo(const LevelZeroDevicePool &device_pool,
                                     const int num_cpu);
            virtual ~LevelZeroAcceleratorTopo() = default;
            int num_accelerator(void) const override;
            int num_accelerator(int domain) const override;
            std::set<int> cpu_affinity_ideal(int accel_idx) const override;
            std::set<int> cpu_affinity_ideal(int domain, int accel_idx) const override;
        private:
            const LevelZeroDevicePool &m_levelzero_device_pool;
            std::vector<std::set<int> > m_cpu_affinity_ideal;
            std::vector<std::set<int> > m_cpu_affinity_ideal_chip;
    };
}
#endif

/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DRMGPUTOPO_HPP_INCLUDE
#define DRMGPUTOPO_HPP_INCLUDE

#include <vector>
#include <set>
#include <string>

#include "GPUTopo.hpp"

namespace geopm
{
    class DrmGpuTopo : public GPUTopo
    {
        public:
            DrmGpuTopo() = delete;
            DrmGpuTopo(const std::string &drm_directory);
            virtual ~DrmGpuTopo() = default;
            int num_gpu(void) const override;
            int num_gpu(int domain) const override;
            std::set<int> cpu_affinity_ideal(int gpu_idx) const override;
            std::set<int> cpu_affinity_ideal(int domain, int gpu_idx) const override;
            std::string gt_path(int gpu_chip_idx) const;
            std::string card_path(int gpu_idx) const;
            std::string driver_name() const;
        private:
            // Name of the driver that this DrmGpuTopo maps
            std::string m_driver_name;
            // Map of (gpu index) -> (drm card path)
            std::vector<std::string> m_card_paths;
            // Map of (gpu_chip index) -> (drm gt path)
            std::vector<std::string> m_gt_paths;
            // Map of (gpu index) -> (set of local cpu indices)
            std::vector<std::set<int> > m_cpu_affinity_by_gpu;
            // Map of (gpu_chip index) -> (gpu index)
            std::vector<int> m_gpu_by_gpu_chip;
    };
}
#endif

/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GPUTOPONULL_HPP_INCLUDE
#define GPUTOPONULL_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <set>

#include "GPUTopo.hpp"

namespace geopm
{
    class GPUTopoNull : public GPUTopo
    {
        public:
            GPUTopoNull() = default ;
            virtual ~GPUTopoNull() = default;
            int num_gpu(void) const override;
            int num_gpu(int domain_type) const override;
            std::set<int> cpu_affinity_ideal(int gpu_idx) const override;
            std::set<int> cpu_affinity_ideal(int domain_type, int gpu_idx) const override;
    };
}
#endif

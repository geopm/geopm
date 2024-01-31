/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GPUTOPO_HPP_INCLUDE
#define GPUTOPO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <set>

namespace geopm
{
    class GPUTopo
    {
        public:
            GPUTopo() = default;
            virtual ~GPUTopo() = default;
            /// @brief Number of GPUs on the platform.
            /// @param [in] domain The GEOPM domain type
            virtual int num_gpu(void) const = 0;
            virtual int num_gpu(int domain) const = 0;
            /// @brief CPU Affinitization set for a particular GPU
            /// @param [in] domain The GEOPM domain type
            /// @param [in] domain_idx The index indicating a particular
            ///        GPU
            virtual std::set<int> cpu_affinity_ideal(int domain_idx) const = 0;
            virtual std::set<int> cpu_affinity_ideal(int domain, int domain_idx) const = 0;
    };

    const GPUTopo &gpu_topo(void);
}
#endif

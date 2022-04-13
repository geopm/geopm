/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ACCELERATORTOPO_HPP_INCLUDE
#define ACCELERATORTOPO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <set>

namespace geopm
{
    class AcceleratorTopo
    {
        public:
            AcceleratorTopo() = default;
            virtual ~AcceleratorTopo() = default;
            /// @brief Number of accelerators on the platform.
            /// @param [in] domain The GEOPM domain type
            virtual int num_accelerator(void) const = 0;
            virtual int num_accelerator(int domain) const = 0;
            /// @brief CPU Affinitization set for a particular accelerator
            /// @param [in] domain The GEOPM domain type
            /// @param [in] domain_idx The index indicating a particular
            ///        accelerator
            virtual std::set<int> cpu_affinity_ideal(int domain_idx) const = 0;
            virtual std::set<int> cpu_affinity_ideal(int domain, int domain_idx) const = 0;
    };

    const AcceleratorTopo &accelerator_topo(void);
}
#endif

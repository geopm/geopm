/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ACCELERATORTOPONULL_HPP_INCLUDE
#define ACCELERATORTOPONULL_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <set>

#include "AcceleratorTopo.hpp"

namespace geopm
{
    class AcceleratorTopoNull : public AcceleratorTopo
    {
        public:
            AcceleratorTopoNull() = default ;
            virtual ~AcceleratorTopoNull() = default;
            int num_accelerator(void) const override;
            int num_accelerator(int domain_type) const override;
            std::set<int> cpu_affinity_ideal(int accel_idx) const override;
            std::set<int> cpu_affinity_ideal(int domain_type, int accel_idx) const override;
    };
}
#endif

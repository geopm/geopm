/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <fstream>
#include <string>

#include "geopm/Exception.hpp"
#include "GPUTopoNull.hpp"
#include "geopm_topo.h"

namespace geopm
{
    int GPUTopoNull::num_gpu(void) const
    {
        return num_gpu(GEOPM_DOMAIN_GPU);
    }

    int GPUTopoNull::num_gpu(int domain_type) const
    {
        return 0;
    }

    std::set<int> GPUTopoNull::cpu_affinity_ideal(int domain_idx) const
    {
        return cpu_affinity_ideal(GEOPM_DOMAIN_GPU, domain_idx);
    }

    std::set<int> GPUTopoNull::cpu_affinity_ideal(int domain_type, int domain_idx) const
    {
        return {};
    }
}

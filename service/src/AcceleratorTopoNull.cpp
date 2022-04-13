/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <fstream>
#include <string>

#include "geopm/Exception.hpp"
#include "AcceleratorTopoNull.hpp"
#include "geopm_topo.h"

namespace geopm
{
    int AcceleratorTopoNull::num_accelerator(void) const
    {
        return num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR);
    }

    int AcceleratorTopoNull::num_accelerator(int domain_type) const
    {
        return 0;
    }

    std::set<int> AcceleratorTopoNull::cpu_affinity_ideal(int domain_idx) const
    {
        return cpu_affinity_ideal(GEOPM_DOMAIN_BOARD_ACCELERATOR, domain_idx);
    }

    std::set<int> AcceleratorTopoNull::cpu_affinity_ideal(int domain_type, int domain_idx) const
    {
        return {};
    }
}

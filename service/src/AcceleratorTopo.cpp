/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <geopm/Helper.hpp>

#include "geopm/Exception.hpp"
#include "AcceleratorTopoNull.hpp"

#include "NVMLAcceleratorTopo.hpp"
#include "LevelZeroAcceleratorTopo.hpp"

namespace geopm
{
    static std::unique_ptr<AcceleratorTopo> make_unique_accelerator_topo(void)
    {
        std::unique_ptr<AcceleratorTopo> result;
        try {
#ifdef GEOPM_ENABLE_NVML
            result = geopm::make_unique<NVMLAcceleratorTopo>();
#elif defined(GEOPM_ENABLE_LEVELZERO)
            result = geopm::make_unique<LevelZeroAcceleratorTopo>();
#else
            result = geopm::make_unique<AcceleratorTopoNull>();
#endif
        }
        catch (const Exception &ex) {
            result = geopm::make_unique<AcceleratorTopoNull>();
        }
        return result;
    }

    const AcceleratorTopo &accelerator_topo(void)
    {
        static std::unique_ptr<AcceleratorTopo> instance = make_unique_accelerator_topo();
        return *instance;
    }

}

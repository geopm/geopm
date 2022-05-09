/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <geopm/Helper.hpp>

#include "geopm/Exception.hpp"
#include "GPUTopoNull.hpp"

#include "NVMLGPUTopo.hpp"
#include "LevelZeroGPUTopo.hpp"

namespace geopm
{
    static std::unique_ptr<GPUTopo> make_unique_gpu_topo(void)
    {
        std::unique_ptr<GPUTopo> result;
        try {
#ifdef GEOPM_ENABLE_NVML
            result = geopm::make_unique<NVMLGPUTopo>();
#elif defined(GEOPM_ENABLE_LEVELZERO)
            result = geopm::make_unique<LevelZeroGPUTopo>();
#else
            result = geopm::make_unique<GPUTopoNull>();
#endif
        }
        catch (const Exception &ex) {
            result = geopm::make_unique<GPUTopoNull>();
        }
        return result;
    }

    const GPUTopo &gpu_topo(void)
    {
        static std::unique_ptr<GPUTopo> instance = make_unique_gpu_topo();
        return *instance;
    }

}

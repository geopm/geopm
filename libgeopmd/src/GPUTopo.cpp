/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <geopm/Helper.hpp>

#include "geopm/Exception.hpp"
#include "GPUTopoNull.hpp"

#include "NVMLGPUTopo.hpp"
#include "LevelZeroGPUTopo.hpp"

namespace geopm
{
    static std::unique_ptr<GPUTopo> make_unique_gpu_topo(void)
    {
        std::unique_ptr<GPUTopo> result = geopm::make_unique<GPUTopoNull>();
        std::unique_ptr<GPUTopo> NVMLTopo = geopm::make_unique<GPUTopoNull>();
        std::unique_ptr<GPUTopo> LevelZeroTopo = geopm::make_unique<GPUTopoNull>();
#ifdef GEOPM_ENABLE_NVML
        try {
            NVMLTopo = geopm::make_unique<NVMLGPUTopo>();
        }
        catch (const Exception &ex) {
            // NVMLTopo cannot be created, already set to GPUTopoNull
        }
#endif
#ifdef GEOPM_ENABLE_LEVELZERO
        try {
            LevelZeroTopo = geopm::make_unique<LevelZeroGPUTopo>();
        }
        catch (const Exception &ex) {
            // LevelZeroTopo cannot be created, already set to GPUTopoNull
        }
#endif
        if (NVMLTopo->num_gpu() != 0 &&
            LevelZeroTopo->num_gpu() != 0) {
            throw Exception("GPUTopo: Discovered GPUs with both NVML and LevelZero, this configuration is not currently supported",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (NVMLTopo->num_gpu() != 0) {
            result = std::move(NVMLTopo);
        }
        else if (LevelZeroTopo->num_gpu() != 0) {
            result = std::move(LevelZeroTopo);
        }
        return result;
    }

    const GPUTopo &gpu_topo(void)
    {
        static std::unique_ptr<GPUTopo> instance = make_unique_gpu_topo();
        return *instance;
    }
}

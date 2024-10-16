/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>

#include <iostream>

#include "geopm/Exception.hpp"

#include "DrmGpuTopo.hpp"
#include "GPUTopoNull.hpp"
#include "LevelZeroGPUTopo.hpp"
#include "NVMLGPUTopo.hpp"

#include <geopm/Helper.hpp>

namespace geopm
{
    static std::unique_ptr<GPUTopo> make_unique_gpu_topo(void)
    {
        std::unique_ptr<GPUTopo> result = std::make_unique<GPUTopoNull>();
        std::unique_ptr<GPUTopo> NVMLTopo = std::make_unique<GPUTopoNull>();
        std::unique_ptr<GPUTopo> LevelZeroTopo = std::make_unique<GPUTopoNull>();
        std::unique_ptr<GPUTopo> DrmTopo = std::make_unique<GPUTopoNull>();
        std::unique_ptr<GPUTopo> AccelTopo = std::make_unique<GPUTopoNull>();

        try {
            DrmTopo = std::make_unique<DrmGpuTopo>("/sys/class/drm");
        }
        catch (const Exception &ex) {
            std::cerr << "Warning: <geopm> Unable to get /sys/class/drm topology. Reason: "
                      << ex.what() << std::endl;
        }

        try {
            std::string accel_path = "/sys/class/accel";
            if (access(accel_path.c_str(), R_OK) == 0) {
                AccelTopo = std::make_unique<DrmGpuTopo>(accel_path);
            }
        }
        catch (const Exception &ex) {
            std::cerr << "Warning: <geopm> Unable to get /sys/class/accel topology. Reason: "
                      << ex.what() << std::endl;
        }
#ifdef GEOPM_ENABLE_NVML
        try {
            NVMLTopo = std::make_unique<NVMLGPUTopo>();
        }
        catch (const Exception &ex) {
            // NVMLTopo cannot be created, already set to GPUTopoNull
        }
#endif
#ifdef GEOPM_ENABLE_LEVELZERO
        if (getenv("ZES_ENABLE_SYSMAN") != nullptr &&
            std::string(getenv("ZES_ENABLE_SYSMAN")) == "1" &&
            getenv("ZE_FLAT_DEVICE_HIERARCHY") != nullptr &&
            std::string(getenv("ZE_FLAT_DEVICE_HIERARCHY")) == "COMPOSITE") {
            try {
                LevelZeroTopo = std::make_unique<LevelZeroGPUTopo>();
            }
            catch (const Exception &ex) {
                // LevelZeroTopo cannot be created, already set to GPUTopoNull
            }
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
        else if (AccelTopo->num_gpu() != 0) {
            result = std::move(AccelTopo);
        }
        else if (DrmTopo->num_gpu() != 0) {
            result = std::move(DrmTopo);
        }

        return result;
    }

    const GPUTopo &gpu_topo(void)
    {
        static std::unique_ptr<GPUTopo> instance = make_unique_gpu_topo();
        return *instance;
    }
}

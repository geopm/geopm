/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <iostream>
#include <string>
#include <map>

#include "config.h"
#include "geopm/Exception.hpp"
#include "LevelZeroDevicePool.hpp"
#include "LevelZeroGPUTopo.hpp"

namespace geopm
{
    LevelZeroGPUTopo::LevelZeroGPUTopo()
        : LevelZeroGPUTopo(levelzero_device_pool(), geopm_sched_num_cpu())
    {
    }

    LevelZeroGPUTopo::LevelZeroGPUTopo(const LevelZeroDevicePool &device_pool,
                                                       const int num_cpu)
        : m_levelzero_device_pool(device_pool)
    {
        unsigned int num_gpu = m_levelzero_device_pool.
                                       num_gpu(GEOPM_DOMAIN_GPU);
        unsigned int num_gpu_chip = m_levelzero_device_pool.
                                            num_gpu(GEOPM_DOMAIN_GPU_CHIP);

        if (num_gpu == 0 || num_gpu_chip == 0) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZeroGPUTopo: No levelZero devices or chips detected.\n";
#endif
        }
        else {
            m_cpu_affinity_ideal.resize(num_gpu);
            unsigned int num_cpu_per_gpu = num_cpu / num_gpu;

            m_cpu_affinity_ideal_chip.resize(num_gpu_chip);
            unsigned int num_chip_per_gpu = num_gpu_chip / num_gpu;

            // TODO: Add ideal cpu to gpu affinitization that isn't a simple split if needed.
            //       This may come from a call to oneAPI, LevelZero, etc
            for (unsigned int gpu_idx = 0; gpu_idx <  num_gpu; ++gpu_idx) {
                int chip_idx = 0;
                for (unsigned int cpu_idx = gpu_idx * num_cpu_per_gpu;
                     cpu_idx < (gpu_idx + 1) * num_cpu_per_gpu;
                     cpu_idx++) {
                    m_cpu_affinity_ideal.at(gpu_idx).insert(cpu_idx);

                    // CHIP to CPU association is currently only used to associate CHIPS to
                    // GPUS.  This logic just distributes the CPUs associated with
                    // an GPU to its CHIPS in a round robin fashion.
                    m_cpu_affinity_ideal_chip.at(gpu_idx * num_chip_per_gpu +
                                                 (chip_idx % num_chip_per_gpu)).insert(cpu_idx);
                    ++chip_idx;
                }
            }
            if ((num_cpu % num_gpu) != 0) {
                unsigned int gpu_idx = 0;
                for (int cpu_idx = num_cpu_per_gpu * num_gpu;
                     cpu_idx < num_cpu; ++cpu_idx) {
                    m_cpu_affinity_ideal.at(gpu_idx % num_gpu).insert(cpu_idx);
                    m_cpu_affinity_ideal_chip.at(gpu_idx * num_chip_per_gpu).insert(cpu_idx);
                    ++gpu_idx;
                }
            }
        }
    }

    int LevelZeroGPUTopo::num_gpu() const
    {
        return num_gpu(GEOPM_DOMAIN_GPU);
    }

    int LevelZeroGPUTopo::num_gpu(int domain) const
    {
        int result = -1;
        if (domain == GEOPM_DOMAIN_GPU) {
            result = m_cpu_affinity_ideal.size();
        }
        else if (domain == GEOPM_DOMAIN_GPU_CHIP) {
            result = m_cpu_affinity_ideal_chip.size();
        }
        else {
            throw Exception("LevelZeroGPUTopo::" + std::string(__func__) + ": domain " +
                            std::to_string(domain) + " is not supported.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    std::set<int> LevelZeroGPUTopo::cpu_affinity_ideal(int gpu_idx) const
    {
        return cpu_affinity_ideal(GEOPM_DOMAIN_GPU, gpu_idx);
    }

    std::set<int> LevelZeroGPUTopo::cpu_affinity_ideal(int domain, int gpu_idx) const
    {
        std::set<int> result = {};
        if (domain == GEOPM_DOMAIN_GPU) {
            if (gpu_idx < 0 || (unsigned int)gpu_idx >= m_cpu_affinity_ideal.size()) {
                throw Exception("LevelZeroGPUTopo::" + std::string(__func__) + ": gpu_idx " +
                                std::to_string(gpu_idx) + " is out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = m_cpu_affinity_ideal.at(gpu_idx);
        }
        else if (domain == GEOPM_DOMAIN_GPU_CHIP) {
            if (gpu_idx < 0 || (unsigned int)gpu_idx >= m_cpu_affinity_ideal_chip.size()) {
                throw Exception("LevelZeroGPUTopo::" + std::string(__func__) + ": gpu_idx " +
                                std::to_string(gpu_idx) + " is out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = m_cpu_affinity_ideal_chip.at(gpu_idx);
        }
        else {
            throw Exception("LevelZeroGPUTopo::" + std::string(__func__) + ": domain " +
                            std::to_string(domain) + " is not supported.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }
}

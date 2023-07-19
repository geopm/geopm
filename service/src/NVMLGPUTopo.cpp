/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>

#include "config.h"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "NVMLDevicePool.hpp"
#include "NVMLGPUTopo.hpp"
#include "geopm_topo.h"

namespace geopm
{
    NVMLGPUTopo::NVMLGPUTopo()
        : NVMLGPUTopo(nvml_device_pool(geopm_sched_num_cpu()), geopm_sched_num_cpu())
    {
    }

    NVMLGPUTopo::NVMLGPUTopo(const NVMLDevicePool &device_pool, const int num_cpu)
        : m_nvml_device_pool(device_pool)
    {
        unsigned int num_gpu = m_nvml_device_pool.num_gpu();
        if (num_gpu == 0) {
            std::cerr << "Warning: <geopm> NVMLGPUTopo: No NVML GPUs detected.\n";
        }
        else {
            int cpu_remaining = 0;
            std::vector<cpu_set_t *> ideal_affinitization_mask_vec;

	    auto affinitized_cpuset = make_cpu_set(num_cpu, {});
            CPU_ZERO_S(CPU_ALLOC_SIZE(num_cpu), affinitized_cpuset.get());

            m_cpu_affinity_ideal.resize(num_gpu);

            // Cache ideal affinitization due to the overhead associated with the NVML calls
            for (unsigned int gpu_idx = 0; gpu_idx <  num_gpu; ++gpu_idx) {
                ideal_affinitization_mask_vec.push_back(m_nvml_device_pool.cpu_affinity_ideal_mask(gpu_idx));
            }

            /// @todo: As an optimization this may be replaceable with CPU_OR of all masks in ideal_affinitzation_mask_vec
            //       and CPU_COUNT of the output
            for (unsigned int gpu_idx = 0; gpu_idx <  num_gpu; ++gpu_idx) {
                for (int cpu_idx = 0; cpu_idx < num_cpu; cpu_idx++) {
                    if (CPU_ISSET(cpu_idx, ideal_affinitization_mask_vec.at(gpu_idx))) {
                        if (CPU_ISSET(cpu_idx, affinitized_cpuset.get()) == 0) {
                            //if this is in this GPU mask and has not
                            //been picked by another GPU
                            CPU_SET(cpu_idx, affinitized_cpuset.get());
                            ++cpu_remaining;
                        }
                    }
                }
            }

            // In order to handle systems where the number of CPUs are not evenly divisble by the number of
            // GPUs a two pass process is used.  This does not guarantee affinitization is successful,
            // fair, or that logical CPUs aren't split between GPUs, but it does cover many common cases
            for (int affinitization_attempts = 0; affinitization_attempts < 2; ++affinitization_attempts) {
                unsigned int num_cpu_per_gpu = cpu_remaining / num_gpu;
                if (num_cpu_per_gpu == 0) {
                    num_cpu_per_gpu = cpu_remaining % num_gpu;
                }

                // This is a greedy approach for mapping CPUs to GPUs, and as such may result in some CPUs
                // not being affinitized at all.  A potential improvement is to always determine affinity
                // for the GPU with the fewest possible CPUs in the GPU mask
                for (unsigned int gpu_idx = 0; gpu_idx <  num_gpu; ++gpu_idx) {
                    unsigned int gpu_cpu_count = 0;
                    for (int cpu_idx = 0;
                         cpu_idx != num_cpu &&
                         gpu_cpu_count < num_cpu_per_gpu;
                         ++cpu_idx) {
                        if (CPU_ISSET(cpu_idx, ideal_affinitization_mask_vec.at(gpu_idx))) {
                            m_cpu_affinity_ideal.at(gpu_idx).insert(cpu_idx);
                            --cpu_remaining;
                            ++gpu_cpu_count;

                            // Remove this CPU from the affinity mask of all GPUs
                            for (unsigned int gpu_idx_inner = 0; gpu_idx_inner <  num_gpu; ++gpu_idx_inner) {
                                CPU_CLR(cpu_idx, ideal_affinitization_mask_vec.at(gpu_idx_inner));
                            }
                        }
                    }
                }
            }

            if (cpu_remaining != 0) {
                throw Exception("NVMLGPUTopo::" + std::string(__func__) +
                                ": Failed to affinitize all valid CPUs to GPUs.  " +
                                std::to_string(cpu_remaining) + " CPUs remain unassociated with any GPU.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    int NVMLGPUTopo::num_gpu(void) const
    {
        return m_cpu_affinity_ideal.size();
    }

    int NVMLGPUTopo::num_gpu(int domain_type) const
    {
        // At this time sub-devices are not supported separate from sub-devices on NVIDIA
        // As such we are reporting a single sub-device per device for mapping purposes
        return num_gpu();
    }

    std::set<int> NVMLGPUTopo::cpu_affinity_ideal(int gpu_idx) const
    {
        return cpu_affinity_ideal(GEOPM_DOMAIN_GPU, gpu_idx);
    }

    std::set<int> NVMLGPUTopo::cpu_affinity_ideal(int domain_type, int gpu_idx) const
    {
        std::set<int> result = {};
        // At this time sub-devices are not supported separate from sub-devices on NVIDIA
        // As such we are reporting a single sub-device per device for mapping purposes
        if (domain_type == GEOPM_DOMAIN_GPU || domain_type == GEOPM_DOMAIN_GPU_CHIP) {
            if (gpu_idx < 0 || gpu_idx >= num_gpu()) {
                throw Exception("NVMLGPUTopo::" + std::string(__func__) + ": gpu_idx " +
                                std::to_string(gpu_idx) + " is out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = m_cpu_affinity_ideal.at(gpu_idx);
        }

        return result;
    }
}

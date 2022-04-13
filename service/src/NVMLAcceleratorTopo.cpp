/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>

#include "config.h"
#include "geopm/Exception.hpp"
#include "NVMLDevicePool.hpp"
#include "NVMLAcceleratorTopo.hpp"
#include "geopm_topo.h"

namespace geopm
{
    NVMLAcceleratorTopo::NVMLAcceleratorTopo()
        : NVMLAcceleratorTopo(nvml_device_pool(geopm_sched_num_cpu()), geopm_sched_num_cpu())
    {
    }

    NVMLAcceleratorTopo::NVMLAcceleratorTopo(const NVMLDevicePool &device_pool, const int num_cpu)
        : m_nvml_device_pool(device_pool)
    {
        unsigned int num_accelerator = m_nvml_device_pool.num_accelerator();
        if (num_accelerator == 0) {
            std::cerr << "Warning: <geopm> NVMLAcceleratorTopo: No NVML accelerators detected.\n";
        }
        else {
            int cpu_remaining = 0;
            std::vector<cpu_set_t *> ideal_affinitization_mask_vec;
            cpu_set_t *affinitized_cpuset = CPU_ALLOC(num_cpu);

            CPU_ZERO_S(CPU_ALLOC_SIZE(num_cpu), affinitized_cpuset);

            m_cpu_affinity_ideal.resize(num_accelerator);

            // Cache ideal affinitization due to the overhead associated with the NVML calls
            for (unsigned int accel_idx = 0; accel_idx <  num_accelerator; ++accel_idx) {
                ideal_affinitization_mask_vec.push_back(m_nvml_device_pool.cpu_affinity_ideal_mask(accel_idx));
            }

            /// @todo: As an optimization this may be replacable with CPU_OR of all masks in ideal_affinitzation_mask_vec
            //       and CPU_COUNT of the output
            for (unsigned int accel_idx = 0; accel_idx <  num_accelerator; ++accel_idx) {
                for (int cpu_idx = 0; cpu_idx < num_cpu; cpu_idx++) {
                    if (CPU_ISSET(cpu_idx, ideal_affinitization_mask_vec.at(accel_idx))) {
                        if (CPU_ISSET(cpu_idx, affinitized_cpuset) == 0) {
                            //if this is in this accelerator mask and has not
                            //been picked by another accelerator
                            CPU_SET(cpu_idx, affinitized_cpuset);
                            ++cpu_remaining;
                        }
                    }
                }
            }

            // In order to handle systems where the number of CPUs are not evenly divisble by the number of
            // Accelerators a two pass process is used.  This does not guarantee affinitization is successful,
            // fair, or that logical CPUs aren't split between Accelerators, but it does cover many common cases
            for (int affinitization_attempts = 0; affinitization_attempts < 2; ++affinitization_attempts) {
                unsigned int num_cpu_per_accelerator = cpu_remaining / num_accelerator;
                if (num_cpu_per_accelerator == 0) {
                    num_cpu_per_accelerator = cpu_remaining % num_accelerator;
                }

                // This is a greedy approach for mapping CPUs to accelerators, and as such may result in some CPUs
                // not being affinitized at all.  A potential improvement is to always determine affinity
                // for the accelerator with the fewest possible CPUs in the accelerator mask
                for (unsigned int accel_idx = 0; accel_idx <  num_accelerator; ++accel_idx) {
                    unsigned int accelerator_cpu_count = 0;
                    for (int cpu_idx = 0;
                         cpu_idx != num_cpu &&
                         accelerator_cpu_count < num_cpu_per_accelerator;
                         ++cpu_idx) {
                        if (CPU_ISSET(cpu_idx, ideal_affinitization_mask_vec.at(accel_idx))) {
                            m_cpu_affinity_ideal.at(accel_idx).insert(cpu_idx);
                            --cpu_remaining;
                            ++accelerator_cpu_count;

                            // Remove this CPU from the affinity mask of all Accelerators
                            for (unsigned int accel_idx_inner = 0; accel_idx_inner <  num_accelerator; ++accel_idx_inner) {
                                CPU_CLR(cpu_idx, ideal_affinitization_mask_vec.at(accel_idx_inner));
                            }
                        }
                    }
                }
            }

            if (cpu_remaining != 0) {
                throw Exception("NVMLAcceleratorTopo::" + std::string(__func__) +
                                ": Failed to affinitize all valid CPUs to Accelerators.  " +
                                std::to_string(cpu_remaining) + " CPUs remain unassociated with any accelerator.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    int NVMLAcceleratorTopo::num_accelerator(void) const
    {
        return m_cpu_affinity_ideal.size();
    }

    int NVMLAcceleratorTopo::num_accelerator(int domain_type) const
    {
        // At this time sub-devices are not supported separate from sub-devices on NVIDIA
        // As such we are reporting a single sub-device per device for mapping purposes
        return num_accelerator();
    }

    std::set<int> NVMLAcceleratorTopo::cpu_affinity_ideal(int accel_idx) const
    {
        return cpu_affinity_ideal(GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
    }

    std::set<int> NVMLAcceleratorTopo::cpu_affinity_ideal(int domain_type, int accel_idx) const
    {
        std::set<int> result = {};
        // At this time sub-devices are not supported separate from sub-devices on NVIDIA
        // As such we are reporting a single sub-device per device for mapping purposes
        if (domain_type == GEOPM_DOMAIN_BOARD_ACCELERATOR || domain_type == GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            if (accel_idx < 0 || accel_idx >= num_accelerator()) {
                throw Exception("NVMLAcceleratorTopo::" + std::string(__func__) + ": accel_idx " +
                                std::to_string(accel_idx) + " is out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = m_cpu_affinity_ideal.at(accel_idx);
        }

        return result;
    }
}

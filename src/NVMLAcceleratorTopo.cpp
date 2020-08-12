/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>

#include "config.h"
#include "Exception.hpp"
#include "NVMLDevicePool.hpp"
#include "NVMLAcceleratorTopo.hpp"

namespace geopm
{
    NVMLAcceleratorTopo::NVMLAcceleratorTopo()
        : NVMLAcceleratorTopo(nvml_device_pool(geopm_sched_num_cpu()), geopm_sched_num_cpu())
    {
    }

    NVMLAcceleratorTopo::NVMLAcceleratorTopo(const NVMLDevicePool &device_pool, const int num_cpu)
        : m_nvml_device_pool(device_pool)
    {
        m_num_accelerator = m_nvml_device_pool.num_accelerator();

        if (m_num_accelerator == 0) {
            std::cerr << "Warning: <geopm> NVMLAcceleratorTopo: No NVML accelerators detected.\n";
        }
        else {
            int cpu_remaining = 0;
            std::vector<cpu_set_t *> ideal_affinitization_mask_vec;
            cpu_set_t *affinitized_cpuset = CPU_ALLOC(num_cpu);

            CPU_ZERO_S(CPU_ALLOC_SIZE(num_cpu), affinitized_cpuset);

            m_ideal_cpu_affinitization.resize(m_num_accelerator);

            // Cache ideal affinitization due to the overhead associated with the NVML calls
            for (int accel_idx = 0; accel_idx <  m_num_accelerator; ++accel_idx) {
                ideal_affinitization_mask_vec.push_back(m_nvml_device_pool.ideal_cpu_affinitization_mask(accel_idx));
            }

            /// @todo: As an optimization this may be replacable with CPU_OR of all masks in ideal_affinitzation_mask_vec
            //       and CPU_COUNT of the output
            for (int accel_idx = 0; accel_idx <  m_num_accelerator; ++accel_idx) {
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
                unsigned int num_cpu_per_accelerator = cpu_remaining / m_num_accelerator;
                if (num_cpu_per_accelerator == 0) {
                    num_cpu_per_accelerator = cpu_remaining % m_num_accelerator;
                }

                // This is a greedy approach for mapping CPUs to accelerators, and as such may result in some CPUs
                // not being affinitized at all.  A potential improvement is to always determine affinity
                // for the accelerator with the fewest possible CPUs in the accelerator mask
                for (int accel_idx = 0; accel_idx <  m_num_accelerator; ++accel_idx) {
                    unsigned int accelerator_cpu_count = 0;
                    int cpu_idx = 0;
                    while (cpu_idx < num_cpu && accelerator_cpu_count < num_cpu_per_accelerator) {
                        if (CPU_ISSET(cpu_idx, ideal_affinitization_mask_vec.at(accel_idx))) {
                            m_ideal_cpu_affinitization.at(accel_idx).insert(cpu_idx);
                            --cpu_remaining;
                            ++accelerator_cpu_count;

                            // Remove this CPU from the affinity mask of all Accelerators
                            for (int accel_idx_inner = 0; accel_idx_inner <  m_num_accelerator; ++accel_idx_inner) {
                                CPU_CLR(cpu_idx, ideal_affinitization_mask_vec.at(accel_idx_inner));
                            }
                        }
                        ++cpu_idx;
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
        return m_num_accelerator;
    }

    std::set<int> NVMLAcceleratorTopo::ideal_cpu_affinitization(int accel_idx) const
    {
        if (accel_idx < 0 || accel_idx >= m_num_accelerator) {
            throw Exception("NVMLAcceleratorTopo::" + std::string(__func__) + ": accel_idx " +
                            std::to_string(accel_idx) + " is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_ideal_cpu_affinitization.at(accel_idx);
    }
}

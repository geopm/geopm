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
#include "LevelZeroDevicePool.hpp"
#include "LevelZeroAcceleratorTopo.hpp"

namespace geopm
{
    LevelZeroAcceleratorTopo::LevelZeroAcceleratorTopo()
        : LevelZeroAcceleratorTopo(levelzero_device_pool(geopm_sched_num_cpu()), geopm_sched_num_cpu())
    {
    }

    LevelZeroAcceleratorTopo::LevelZeroAcceleratorTopo(const LevelZeroDevicePool &device_pool, const int num_cpu)
        : m_levelzero_device_pool(device_pool)
        , m_num_accelerator(m_levelzero_device_pool.num_accelerator())
    {
        if (m_num_accelerator == 0) {
            std::cerr << "Warning: <geopm> LevelZeroAcceleratorTopo: No levelZero accelerators detected.\n";
        }
        else {
            m_cpu_affinity_ideal.resize(m_num_accelerator);
            unsigned int num_cpu_per_accelerator = num_cpu / m_num_accelerator;

            //TODO: Add ideal cpu to accelerator affinitization that isn't a simple split.  This may come from
            //      a call to oneAPI
            for (unsigned int accel_idx = 0; accel_idx <  m_num_accelerator; ++accel_idx) {
                for (unsigned int cpu_idx = accel_idx*num_cpu_per_accelerator;
                     cpu_idx < (accel_idx+1)*num_cpu_per_accelerator;
                     cpu_idx++) {
                    m_cpu_affinity_ideal.at(accel_idx).insert(cpu_idx);
                }
            }

            if ((num_cpu % m_num_accelerator) != 0) {
                unsigned int accel_idx = 0;
                for (int cpu_idx = num_cpu_per_accelerator*m_num_accelerator; cpu_idx < num_cpu; ++cpu_idx) {
                    m_cpu_affinity_ideal.at(accel_idx%m_num_accelerator).insert(cpu_idx);
                    ++accel_idx;
                }
            }
        }
    }

    int LevelZeroAcceleratorTopo::num_accelerator(void) const
    {
        return m_num_accelerator;
    }

    std::set<int> LevelZeroAcceleratorTopo::cpu_affinity_ideal(int accel_idx) const
    {
        if (accel_idx < 0 || (unsigned int)accel_idx >= m_num_accelerator) {
            throw Exception("LevelZeroAcceleratorTopo::" + std::string(__func__) + ": accel_idx " +
                            std::to_string(accel_idx) + " is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_cpu_affinity_ideal.at(accel_idx);
    }
}

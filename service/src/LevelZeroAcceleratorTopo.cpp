/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include <iostream>
#include <string>
#include <map>

#include "config.h"
#include "geopm/Exception.hpp"
#include "LevelZeroDevicePool.hpp"
#include "LevelZeroAcceleratorTopo.hpp"

namespace geopm
{
    LevelZeroAcceleratorTopo::LevelZeroAcceleratorTopo()
        : LevelZeroAcceleratorTopo(levelzero_device_pool(), geopm_sched_num_cpu())
    {
    }

    LevelZeroAcceleratorTopo::LevelZeroAcceleratorTopo(const LevelZeroDevicePool &device_pool,
                                                       const int num_cpu)
        : m_levelzero_device_pool(device_pool)
    {
        unsigned int num_accelerator = m_levelzero_device_pool.
                                       num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR);
        unsigned int num_accelerator_chip = m_levelzero_device_pool.
                                            num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP);

        if (num_accelerator == 0 || num_accelerator_chip == 0) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZeroAcceleratorTopo: No levelZero devices or chips detected.\n";
#endif
        }
        else {
            m_cpu_affinity_ideal.resize(num_accelerator);
            unsigned int num_cpu_per_accelerator = num_cpu / num_accelerator;

            m_cpu_affinity_ideal_chip.resize(num_accelerator_chip);
            unsigned int num_chip_per_accelerator = num_accelerator_chip / num_accelerator;

            // TODO: Add ideal cpu to accelerator affinitization that isn't a simple split if needed.
            //       This may come from a call to oneAPI, LevelZero, etc
            for (unsigned int accel_idx = 0; accel_idx <  num_accelerator; ++accel_idx) {
                int chip_idx = 0;
                for (unsigned int cpu_idx = accel_idx * num_cpu_per_accelerator;
                     cpu_idx < (accel_idx + 1) * num_cpu_per_accelerator;
                     cpu_idx++) {
                    m_cpu_affinity_ideal.at(accel_idx).insert(cpu_idx);

                    // CHIP to CPU association is currently only used to associate CHIPS to
                    // ACCELERATORS.  This logic just distributes the CPUs associated with
                    // an ACCELERATOR to its CHIPS in a round robin fashion.
                    m_cpu_affinity_ideal_chip.at(accel_idx * num_chip_per_accelerator +
                                                 (chip_idx % num_chip_per_accelerator)).insert(cpu_idx);
                    ++chip_idx;
                }
            }
            if ((num_cpu % num_accelerator) != 0) {
                unsigned int accel_idx = 0;
                for (int cpu_idx = num_cpu_per_accelerator * num_accelerator;
                     cpu_idx < num_cpu; ++cpu_idx) {
                    m_cpu_affinity_ideal.at(accel_idx % num_accelerator).insert(cpu_idx);
                    m_cpu_affinity_ideal_chip.at(accel_idx * num_chip_per_accelerator).insert(cpu_idx);
                    ++accel_idx;
                }
            }
        }
    }

    int LevelZeroAcceleratorTopo::num_accelerator() const
    {
        return num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR);
    }

    int LevelZeroAcceleratorTopo::num_accelerator(int domain) const
    {
        int result = -1;
        if (domain == GEOPM_DOMAIN_BOARD_ACCELERATOR) {
            result = m_cpu_affinity_ideal.size();
        }
        else if (domain == GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            result = m_cpu_affinity_ideal_chip.size();
        }
        else {
            throw Exception("LevelZeroAcceleratorTopo::" + std::string(__func__) + ": domain " +
                            std::to_string(domain) + " is not supported.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    std::set<int> LevelZeroAcceleratorTopo::cpu_affinity_ideal(int accel_idx) const
    {
        return cpu_affinity_ideal(GEOPM_DOMAIN_BOARD_ACCELERATOR, accel_idx);
    }

    std::set<int> LevelZeroAcceleratorTopo::cpu_affinity_ideal(int domain, int accel_idx) const
    {
        std::set<int> result = {};
        if (domain == GEOPM_DOMAIN_BOARD_ACCELERATOR) {
            if (accel_idx < 0 || (unsigned int)accel_idx >= m_cpu_affinity_ideal.size()) {
                throw Exception("LevelZeroAcceleratorTopo::" + std::string(__func__) + ": accel_idx " +
                                std::to_string(accel_idx) + " is out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = m_cpu_affinity_ideal.at(accel_idx);
        }
        else if (domain == GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            if (accel_idx < 0 || (unsigned int)accel_idx >= m_cpu_affinity_ideal_chip.size()) {
                throw Exception("LevelZeroAcceleratorTopo::" + std::string(__func__) + ": accel_idx " +
                                std::to_string(accel_idx) + " is out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = m_cpu_affinity_ideal_chip.at(accel_idx);
        }
        else {
            throw Exception("LevelZeroAcceleratorTopo::" + std::string(__func__) + ": domain " +
                            std::to_string(domain) + " is not supported.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }
}

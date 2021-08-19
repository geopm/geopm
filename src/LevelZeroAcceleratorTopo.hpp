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

#ifndef LEVELZEROACCELERATORTOPO_HPP_INCLUDE
#define LEVELZEROACCELERATORTOPO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <set>

#include "AcceleratorTopo.hpp"

namespace geopm
{
    class LevelZeroDevicePool;

    class LevelZeroAcceleratorTopo : public AcceleratorTopo
    {
        public:
            LevelZeroAcceleratorTopo();
            LevelZeroAcceleratorTopo(const LevelZeroDevicePool &device_pool,
                                     const int num_cpu);
            virtual ~LevelZeroAcceleratorTopo() = default;
            int num_accelerator(void) const override;
            int num_accelerator(int domain) const override;
            std::set<int> cpu_affinity_ideal(int accel_idx) const override;
            std::set<int> cpu_affinity_ideal(int domain, int accel_idx) const override;
        private:
            const LevelZeroDevicePool &m_levelzero_device_pool;
            std::vector<std::set<int> > m_cpu_affinity_ideal;
            std::vector<std::set<int> > m_cpu_affinity_ideal_chip;
    };
}
#endif

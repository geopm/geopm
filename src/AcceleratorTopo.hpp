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

#ifndef ACCELERATORTOPO_HPP_INCLUDE
#define ACCELERATORTOPO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <set>

namespace geopm
{
    class AcceleratorTopo
    {
        public:
            AcceleratorTopo() = default;
            virtual ~AcceleratorTopo() = default;
            /// @brief Number of accelerators on the platform.
            virtual int num_accelerator(void) const = 0;
            /// @brief Number of accelerator subdevices on the platform.
            virtual int num_accelerator_subdevice(void) const = 0;
            /// @brief CPU Affinitization set for a particular accelerator
            /// @param [in] domain_idx The index indicating a particular
            ///        accelerator
            virtual std::set<int> cpu_affinity_ideal(int domain_idx) const = 0;
            /// @brief CPU Affinitization set for a particular accelerator subdevice
            /// @param [in] domain_idx The index indicating a particular
            ///        accelerator subdevice
            virtual std::set<int> cpu_affinity_ideal_subdevice(int domain_idx) const = 0;
    };

    const AcceleratorTopo &accelerator_topo(void);
}
#endif

/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#ifndef PROFILEIORUNTIME_HPP_INCLUDE
#define PROFILEIORUNTIME_HPP_INCLUDE

#include <cstdint>

#include <vector>
#include <map>
#include <set>

#include "config.h"

namespace geopm
{
    class RuntimeRegulator;

    class IProfileIORuntime
    {
        public:
            IProfileIORuntime() = default;
            virtual ~IProfileIORuntime() = default;
            virtual void insert_regulator(uint64_t region_id, RuntimeRegulator &reg) = 0;
            virtual std::vector<double> per_cpu_runtime(uint64_t region_id) = 0;
    };

    class ProfileIORuntime : public IProfileIORuntime
    {
        public:
            ProfileIORuntime(const std::vector<int> &cpu_rank);
            virtual ~ProfileIORuntime() = default;
            void insert_regulator(uint64_t region_id, RuntimeRegulator &reg) override;
            std::vector<double> per_cpu_runtime(uint64_t region_id) override;
        protected:
            /// @brief The rank index of the rank running on each CPU.
            std::vector<int> m_cpu_rank;
            /// @brief A map from the MPI rank reported in the
            ///        ProfileSampler data to the node local rank
            ///        index.
            std::map<int, int> m_rank_idx_map;

            std::map<uint64_t, RuntimeRegulator&> m_regulators;
    };
}

#endif

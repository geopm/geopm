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

#include "ProfileIORuntime.hpp"
#include "ProfileIO.hpp"
#include "RuntimeRegulator.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    ProfileIORuntime::ProfileIORuntime(const std::vector<int> &cpu_rank)
    {
        m_cpu_rank = ProfileIO::rank_to_node_local_rank_per_cpu(cpu_rank);
    }

    void ProfileIORuntime::insert_regulator(uint64_t region_id, IRuntimeRegulator &reg)
    {
        m_regulator.emplace(std::pair<uint64_t, IRuntimeRegulator&>(region_id, reg));
    }

    std::vector<double> ProfileIORuntime::per_cpu_runtime(uint64_t region_id) const
    {
        std::vector<double> result(m_cpu_rank.size(), 0.0);
#ifdef GEOPM_DEBUG
        if (m_regulator.find(region_id) == m_regulator.end()) {
            throw Exception("ProfileIORuntime::per_cpu_runtime: No regulator set "
                            "for region " + std::to_string(region_id),
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        auto rank_runtimes = m_regulator.at(region_id).runtimes();
        int cpu_idx = 0;
        for (auto rank : m_cpu_rank) {
#ifdef GEOPM_DEBUG
            if (rank > (int)rank_runtimes.size() - 1) {
                throw Exception("ProfileIORuntime::per_cpu_runtime: node-local rank "
                                "for rank " + std::to_string(rank) + " not found in map.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            result[cpu_idx] = rank_runtimes[rank];
            ++cpu_idx;
        }
        return result;
    }
}

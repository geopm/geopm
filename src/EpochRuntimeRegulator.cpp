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

#include <algorithm>

#include "geopm.h"
#include "geopm_message.h"
#include "Exception.hpp"
#include "Helper.hpp"
#include "KruntimeRegulator.hpp"
#include "EpochRuntimeRegulator.hpp"
#include "PlatformIO.hpp"

#include "config.h"

namespace geopm
{
    EpochRuntimeRegulator::EpochRuntimeRegulator(int rank_per_node)
        : m_rank_per_node(rank_per_node)
        , m_curr_ignore_runtime(m_rank_per_node, 0.0)
        , m_curr_mpi_runtime(m_rank_per_node, 0.0)
        , m_last_epoch_runtime(m_rank_per_node, 0.0)
        , m_agg_runtime(m_rank_per_node, 0.0)
    {
        if (m_rank_per_node <= 0) {
            throw Exception("EpochRuntimeRegulator::EpochRuntimeRegulator(): invalid max rank count", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        /// @todo bring back MPIKruntimeRegulator (renamed though)
        m_rid_regulator_map.emplace(std::piecewise_construct,
                                    std::make_tuple(GEOPM_REGION_ID_EPOCH),
                                    std::make_tuple(geopm::make_unique<KruntimeRegulator>
                                                    (m_rank_per_node)));
        m_rid_regulator_map.emplace(std::piecewise_construct,
                                    std::make_tuple(GEOPM_REGION_ID_MPI),
                                    std::make_tuple(geopm::make_unique<KruntimeRegulator>
                                                    (m_rank_per_node)));
    }

    EpochRuntimeRegulator::~EpochRuntimeRegulator() = default;

    void EpochRuntimeRegulator::record_entry(uint64_t region_id, int rank, struct geopm_time_s entry_time)
    {
        auto reg_it = m_rid_regulator_map.emplace(std::piecewise_construct,
                                                  std::forward_as_tuple(region_id),
                                                  std::forward_as_tuple(geopm::make_unique<KruntimeRegulator>
                                                                        (m_rank_per_node)));
        reg_it.first->second->record_entry(rank, entry_time);
        if (geopm_region_id_is_mpi(region_id)) {
            m_rid_regulator_map.find(GEOPM_REGION_ID_MPI)->second->record_entry(rank, entry_time);
        }
    }

    void EpochRuntimeRegulator::record_exit(uint64_t region_id, int rank, struct geopm_time_s exit_time)
    {
        auto reg_it = m_rid_regulator_map.find(region_id);
        if (reg_it == m_rid_regulator_map.end()) {
/// @todo...
#if 0
#ifdef GEOPM_DEBUG
        else if (rid != GEOPM_REGION_ID_EPOCH &&
                rid != GEOPM_REGION_ID_UNMARKED &&
                !geopm_region_id_hint_is_equal(GEOPM_REGION_HINT_IGNORE, rid)) {
            throw Exception("EpochRuntimeRegulator::record_exit(): unknown region detected.", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
#endif
        }
        reg_it->second->record_exit(rank, exit_time);
        if (geopm_region_id_is_mpi(region_id)) {
            m_rid_regulator_map.find(GEOPM_REGION_ID_MPI)->second->record_exit(rank, exit_time);
        }
        if (rank < 0 || rank >= m_rank_per_node) {
            throw Exception("EpochRuntimeRegulator::record_exit(): invalid rank value", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        switch (region_id) {
            case GEOPM_REGION_ID_EPOCH:
                m_last_epoch_runtime[rank] = reg_it->second->per_rank_last_runtime()[rank];
                m_last_epoch_runtime[rank] = m_last_epoch_runtime[rank] - (m_curr_mpi_runtime[rank] + m_curr_ignore_runtime[rank]);
                m_agg_runtime[rank] += m_last_epoch_runtime[rank];
                m_curr_mpi_runtime[rank] = 0.0;
                m_curr_ignore_runtime[rank] = 0.0;
                break;
            case GEOPM_REGION_ID_MPI:
                m_curr_mpi_runtime[rank] += reg_it->second->per_rank_last_runtime()[rank];
                break;
            default:
                if (geopm_region_id_hint_is_equal(GEOPM_REGION_HINT_IGNORE, region_id)) {
                    m_curr_ignore_runtime[rank] += reg_it->second->per_rank_last_runtime()[rank];
                }
        }
    }

    /// @todo shouldn't need...unless team is fine with exposing ref
    const IKruntimeRegulator &EpochRuntimeRegulator::region_regulator(uint64_t region_id) const
    {
        auto reg_it = m_rid_regulator_map.find(region_id);
        if (reg_it == m_rid_regulator_map.end()) {
            /// @todo throw
        }
        return *(reg_it->second.get());
    }

    std::vector<double> EpochRuntimeRegulator::last_epoch_time() const
    {
        return m_last_epoch_runtime;
    }

    std::vector<double> EpochRuntimeRegulator::per_rank_last_runtime(uint64_t region_id) const
    {
        auto reg_it = m_rid_regulator_map.find(region_id);
        if (reg_it == m_rid_regulator_map.end()) {
#ifdef GEOPM_DEBUG
            throw Exception("EpochRuntimeRegulator::runtimes(): unknown region detected.", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
            return {};
        }
        return reg_it->second->per_rank_last_runtime();
    }

    double EpochRuntimeRegulator::total_region_runtime(uint64_t region_id) const
    {
        if (region_id == GEOPM_REGION_ID_EPOCH) {
            return IPlatformIO::agg_average(m_agg_runtime);
        }

        return IPlatformIO::agg_average(region_regulator(region_id).per_rank_total_runtime());
    }

    double EpochRuntimeRegulator::total_region_mpi_time(uint64_t region_id) const
    {
        region_id = geopm_region_id_set_mpi(region_id);
        return total_region_runtime(region_id);
    }

    double EpochRuntimeRegulator::total_epoch_runtime(void) const
    {
        return total_region_runtime(GEOPM_REGION_ID_EPOCH);
    }

#if 0
    double EpochRuntimeRegulator::last_epoch_time(void) const
    {
        return NAN;
    }
#endif

    int EpochRuntimeRegulator::total_count(uint64_t region_id) const
    {
        int result = 0;
        auto rank_count = region_regulator(region_id).per_rank_count();
        if (rank_count.size() != 0) {
            result = *std::max_element(rank_count.begin(), rank_count.end());
        }
        return result;
    }
}

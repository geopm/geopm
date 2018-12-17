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

#include <set>
#include <algorithm>

#include "geopm_region_id.h"

#include "EpochRuntimeRegulator.hpp"
#include "ProfileIOSample.hpp"
#include "CircularBuffer.hpp"
#include "RuntimeRegulator.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    ProfileIOSample::ProfileIOSample(const std::vector<int> &cpu_rank, IEpochRuntimeRegulator &epoch_regulator)
        : m_epoch_regulator(epoch_regulator)
        , m_thread_progress(cpu_rank.size(), NAN)
    {
        // This object is created when app connects
        geopm_time(&m_app_start_time);

        // Ths following is necessary because all other usages of "time zero" query the TimeIOGroup.
        double elapsed = platform_io().read_signal("TIME", PlatformTopo::M_DOMAIN_BOARD, 0);
        geopm_time_add(&m_app_start_time, elapsed * -1, &m_app_start_time);

        m_rank_idx_map = rank_to_node_local_rank(cpu_rank);
        m_cpu_rank = rank_to_node_local_rank_per_cpu(cpu_rank);
        m_num_rank = m_rank_idx_map.size();

        // 2 samples for linear interpolation
        m_rank_sample_buffer.resize(m_num_rank, CircularBuffer<struct m_rank_sample_s>(2));
        m_region_id.resize(m_num_rank, GEOPM_REGION_ID_UNMARKED);
    }

    ProfileIOSample::~ProfileIOSample()
    {

    }

    std::map<int, int> ProfileIOSample::rank_to_node_local_rank(const std::vector<int> &per_cpu_rank)
    {
        std::set<int> rank_set;
        for (auto rank : per_cpu_rank) {
            if (rank != -1) {
                rank_set.insert(rank);
            }
        }
        std::map<int, int> rank_idx_map;
        int i = 0;
        for (auto rank : rank_set) {
            rank_idx_map.insert(std::pair<int, int>(rank, i));
            ++i;
        }
        return rank_idx_map;
    }

    std::vector<int> ProfileIOSample::rank_to_node_local_rank_per_cpu(const std::vector<int> &per_cpu_rank)
    {
        std::vector<int> result(per_cpu_rank);
        std::map<int, int> rank_idx_map = rank_to_node_local_rank(per_cpu_rank);
        for (auto &rank_it : result) {
            auto node_local_rank_it = rank_idx_map.find(rank_it);
            rank_it = node_local_rank_it->second;
        }
        return result;
    }

    void ProfileIOSample::finalize_unmarked_region()
    {
        struct geopm_time_s time;
        /// @todo This time should come from the application.
        geopm_time(&time);
        for (int rank = 0; rank < (int)m_region_id.size(); ++rank) {
            if (m_region_id[rank] == GEOPM_REGION_ID_UNMARKED) {
                m_epoch_regulator.record_exit(GEOPM_REGION_ID_UNMARKED, rank, time);
            }
            m_epoch_regulator.epoch(rank, time);
        }
    }

    void ProfileIOSample::update(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                 std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end)
    {
        for (auto sample_it = prof_sample_begin; sample_it != prof_sample_end; ++sample_it) {
            auto rank_idx_it = m_rank_idx_map.find(sample_it->second.rank);
#ifdef GEOPM_DEBUG
            if (rank_idx_it == m_rank_idx_map.end()) {
                throw Exception("ProfileIOSample::update(): invalid profile sample data",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            size_t local_rank = rank_idx_it->second;
            uint64_t region_id = sample_it->second.region_id;
            if (geopm_region_id_is_epoch(region_id)) {
                m_epoch_regulator.epoch(local_rank, sample_it->second.timestamp);
            }
            else {
                struct m_rank_sample_s rank_sample { .timestamp = sample_it->second.timestamp,
                                                     .progress = sample_it->second.progress };
                if (m_region_id[local_rank] != region_id) {
                    if (rank_sample.progress == 0.0) {
                        if (m_region_id[local_rank] == GEOPM_REGION_ID_UNMARKED) {
                            m_epoch_regulator.record_exit(GEOPM_REGION_ID_UNMARKED, local_rank, rank_sample.timestamp);
                        }
                        m_epoch_regulator.record_entry(region_id, local_rank, rank_sample.timestamp);
                    }
                    m_rank_sample_buffer[local_rank].clear();
                }
                if (rank_sample.progress == 1.0) {
                    m_epoch_regulator.record_exit(region_id, local_rank, rank_sample.timestamp);
                    uint64_t mpi_parent_rid = geopm_region_id_unset_mpi(region_id);
                    if (m_epoch_regulator.is_regulated(mpi_parent_rid)) {
                        m_region_id[local_rank] = mpi_parent_rid;
                    }
                    else {
                        if (m_region_id[local_rank] != GEOPM_REGION_ID_UNMARKED) {
                            m_region_id[local_rank] = GEOPM_REGION_ID_UNMARKED;
                            m_epoch_regulator.record_entry(GEOPM_REGION_ID_UNMARKED, local_rank, rank_sample.timestamp);
                        }
                    }
                }
                else {
                    m_region_id[local_rank] = region_id;
                }
                m_rank_sample_buffer[local_rank].insert(rank_sample);
            }
        }
    }

    void ProfileIOSample::update_thread(const std::vector<double> &thread_progress)
    {
        m_thread_progress = thread_progress;
    }

    std::vector<double> ProfileIOSample::per_cpu_progress(const struct geopm_time_s &extrapolation_time) const
    {
        std::vector<double> result(m_cpu_rank.size(), 0.0);
        std::vector<double> rank_progress = per_rank_progress(extrapolation_time);
        int cpu_idx = 0;
        for (auto it : m_cpu_rank) {
            result[cpu_idx] = rank_progress[it];
            ++cpu_idx;
        }
        return result;
    }

    std::vector<double> ProfileIOSample::per_cpu_thread_progress(void) const
    {
        return m_thread_progress;
    }

    std::vector<double> ProfileIOSample::per_rank_progress(const struct geopm_time_s &extrapolation_time) const
    {
        double delta;
        double factor;
        double dsdt;
        geopm_time_s timestamp_prev[2];
        std::vector<double> result(m_num_rank);
#ifdef GEOPM_DEBUG
        if (m_rank_sample_buffer.size() != m_num_rank) {
            throw Exception("m_rank_sample_buffer was wrong size", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif

        auto result_it = result.begin();
        for (auto sample_it = m_rank_sample_buffer.begin();
             sample_it != m_rank_sample_buffer.end();
             ++sample_it, ++result_it) {
            switch(sample_it->size()) {
                case M_INTERP_TYPE_NONE:
                    *result_it = 0.0;
                    break;
                case M_INTERP_TYPE_NEAREST:
                    // if there is only one sample insert it directly
                    *result_it = sample_it->value(0).progress;
                    break;
                case M_INTERP_TYPE_LINEAR:
                    // if there are two samples, extrapolate to the given timestamp
                    timestamp_prev[0] = sample_it->value(0).timestamp;
                    timestamp_prev[1] = sample_it->value(1).timestamp;
                    delta = geopm_time_diff(timestamp_prev + 1, &extrapolation_time);
                    factor = 1.0 / geopm_time_diff(timestamp_prev, timestamp_prev + 1);
                    dsdt = (sample_it->value(1).progress - sample_it->value(0).progress) * factor;
                    dsdt = dsdt > 0.0 ? dsdt : 0.0; // progress does not decrease over time
                    if (sample_it->value(1).progress == 1.0) {
                        *result_it = 1.0;
                    }
                    else if (sample_it->value(0).progress == 0.0) {
                        // so we don't miss region entry
                        *result_it = 0.0;
                    }
                    else {
                        *result_it = sample_it->value(1).progress + dsdt * delta;
                        *result_it = *result_it >= 0.0 ? *result_it : 1e-9;
                        *result_it = *result_it <= 1.0 ? *result_it : 1 - 1e-9;
                    }
                    break;
                default:
#ifdef GEOPM_DEBUG
                    throw Exception("ProfileIOSample::align_prof() CircularBuffer has more than two values",
                                    GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
                    break;
            }
        }
        return result;
    }

    std::vector<uint64_t> ProfileIOSample::per_cpu_region_id(void) const
    {
        std::vector<uint64_t> result(m_cpu_rank.size(), GEOPM_REGION_ID_UNMARKED);
        int cpu_idx = 0;
        for (auto rank : m_cpu_rank) {
            result[cpu_idx] = m_region_id[rank];
            ++cpu_idx;
        }
        return result;
    }

    std::vector<double> ProfileIOSample::per_cpu_runtime(uint64_t region_id) const
    {
        std::vector<double> result(m_cpu_rank.size(), 0.0);
        const std::vector<double> &rank_runtimes = m_epoch_regulator.region_regulator(region_id).per_rank_last_runtime();
        int cpu_idx = 0;
        for (auto rank : m_cpu_rank) {
#ifdef GEOPM_DEBUG
            if (rank >= (int)rank_runtimes.size()) {
                throw Exception("ProfileIOSample::per_cpu_runtime: node-local rank "
                                "for rank " + std::to_string(rank) + " not found in map.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            result[cpu_idx] = rank_runtimes[rank];
            ++cpu_idx;
        }
        return result;
    }

    double ProfileIOSample::total_app_runtime(void) const
    {
        return geopm_time_since(&m_app_start_time);
    }

    std::vector<int> ProfileIOSample::cpu_rank(void) const
    {
        return m_cpu_rank;
    }
}

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
#include <fstream>

#include "EpochRuntimeRegulator.hpp"
#include "KprofileIOSample.hpp"
#include "ProfileIO.hpp"
#include "CircularBuffer.hpp"
#include "KruntimeRegulator.hpp"
#include "PlatformIO.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    KprofileIOSample::KprofileIOSample(const std::vector<int> &cpu_rank, IEpochRuntimeRegulator &epoch_regulator)
        : m_epoch_regulator(epoch_regulator)
    {
        // This object is created when app connects
        geopm_time(&m_app_start_time);

        m_rank_idx_map = ProfileIO::rank_to_node_local_rank(cpu_rank);
        m_cpu_rank = ProfileIO::rank_to_node_local_rank_per_cpu(cpu_rank);
        m_num_rank = m_rank_idx_map.size();

        // 2 samples for linear interpolation
        m_rank_sample_buffer.resize(m_num_rank, CircularBuffer<struct m_rank_sample_s>(2));
        m_region_id.resize(m_num_rank, GEOPM_REGION_ID_UNMARKED);
    }

    KprofileIOSample::~KprofileIOSample()
    {

    }

    void KprofileIOSample::finalize_unmarked_region()
    {
        struct geopm_time_s time;
        geopm_time(&time);
        for (int rank = 0; rank < (int)m_region_id.size(); ++rank) {
            if (m_region_id[rank] == GEOPM_REGION_ID_UNMARKED) {
                m_epoch_regulator.record_exit(GEOPM_REGION_ID_UNMARKED, rank, time);
            }
            m_epoch_regulator.epoch(rank, time);
        }
    }

    void KprofileIOSample::update(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                  std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end)
    {
        static std::ofstream dump_file;
        dump_file.open("/tmp/profile_table_dump.txt", std::ios::app);

        for (auto sample_it = prof_sample_begin; sample_it != prof_sample_end; ++sample_it) {
            dump_file << "rank: " << sample_it->second.rank << " rid: " << sample_it->second.region_id
                      << " progress: " << sample_it->second.progress << std::endl;

            auto rank_idx_it = m_rank_idx_map.find(sample_it->second.rank);
#ifdef GEOPM_DEBUG
            if (rank_idx_it == m_rank_idx_map.end()) {
                throw Exception("KprofileIOSample::update(): invalid profile sample data",
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
                dump_file << "rank rid set to " << m_region_id[local_rank] << std::endl;
                m_rank_sample_buffer[local_rank].insert(rank_sample);
            }
        }
        dump_file.close();
    }

    std::vector<double> KprofileIOSample::per_cpu_progress(const struct geopm_time_s &extrapolation_time) const
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

    std::vector<double> KprofileIOSample::per_rank_progress(const struct geopm_time_s &extrapolation_time) const
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
                    throw Exception("KprofileIOSample::align_prof() CircularBuffer has more than two values",
                                    GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
                    break;
            }
        }
        return result;
    }

    std::vector<uint64_t> KprofileIOSample::per_cpu_region_id(void) const
    {
        std::vector<uint64_t> result(m_cpu_rank.size(), GEOPM_REGION_ID_UNMARKED);
        int cpu_idx = 0;
        for (auto rank : m_cpu_rank) {
            result[cpu_idx] = m_region_id[rank];
            ++cpu_idx;
        }
        return result;
    }

    std::vector<double> KprofileIOSample::per_cpu_runtime(uint64_t region_id) const
    {
        std::vector<double> result(m_cpu_rank.size(), 0.0);
        const std::vector<double> &rank_runtimes = m_epoch_regulator.region_regulator(region_id).per_rank_last_runtime();
        int cpu_idx = 0;
        for (auto rank : m_cpu_rank) {
#ifdef GEOPM_DEBUG
            if (rank >= (int)rank_runtimes.size()) {
                throw Exception("KprofileIOSample::per_cpu_runtime: node-local rank "
                                "for rank " + std::to_string(rank) + " not found in map.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            result[cpu_idx] = rank_runtimes[rank];
            ++cpu_idx;
        }
        return result;
    }

    double KprofileIOSample::total_app_runtime(void) const
    {
        geopm_time_s curr_time{{0, 0}};
        geopm_time(&curr_time);
        return geopm_time_diff(&m_app_start_time, &curr_time);
    }

    std::list<std::pair<uint64_t, double> > KprofileIOSample::region_entry_exit(void) const
    {
        return m_region_entry_exit;
    }

    void KprofileIOSample::clear_region_entry_exit(void)
    {
        m_region_entry_exit.clear();
    }
}

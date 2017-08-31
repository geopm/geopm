/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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
#include <string.h>

#include "SampleRegulator.hpp"
#include "CircularBuffer.hpp"
#include "config.h"

namespace geopm
{
    SampleRegulator::SampleRegulator(const std::vector<int> &cpu_rank)
    {
        std::set<int> rank_set;
        for (auto it = cpu_rank.begin(); it != cpu_rank.end(); ++it) {
            if ((*it) != -1) {
                rank_set.insert(*it);
            }
        }
        m_num_rank = rank_set.size();
        int i = 0;
        for (auto it = rank_set.begin(); it != rank_set.end(); ++it) {
            m_rank_idx_map.insert(std::pair<int, int>(*it, i));
            ++i;
        }
        for (int i = 0; i < m_num_rank; ++i) {
            m_rank_sample_prev.push_back(new CircularBuffer<struct m_rank_sample_s>(M_INTERP_TYPE_LINEAR)); // two samples are required for linear interpolation
        }
        m_region_id.resize(m_num_rank, GEOPM_REGION_ID_UNMARKED);
    }

    SampleRegulator::~SampleRegulator()
    {
        for (auto it = m_rank_sample_prev.begin(); it != m_rank_sample_prev.end(); ++it) {
            delete *it;
        }
    }

    void SampleRegulator::operator () (const struct geopm_time_s &platform_sample_time,
                                       std::vector<double>::const_iterator platform_sample_begin,
                                       std::vector<double>::const_iterator platform_sample_end,
                                       std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                       std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end,
                                       std::vector<double> &aligned_signal,
                                       std::vector<uint64_t> &region_id)
    {
        // Insert new application profile data into buffers
        insert(prof_sample_begin, prof_sample_end);

        // Populate class member with input platform data
        insert(platform_sample_begin, platform_sample_end);

        // Extrapolate application profile data to time of platform telemetry sample
        align(platform_sample_time);

        aligned_signal = m_aligned_signal;
        region_id = m_region_id;
    }

    const std::map<int, int> &SampleRegulator::rank_idx_map(void) const
    {
        return m_rank_idx_map;
    }

    void SampleRegulator::insert(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                 std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end)
    {
        if (prof_sample_begin != prof_sample_end) {
            for (auto it = prof_sample_begin; it != prof_sample_end; ++it) {
                if (!geopm_region_id_is_epoch((*it).second.region_id) &&
                    (*it).second.region_id != GEOPM_REGION_ID_UNMARKED) {
                    struct m_rank_sample_s rank_sample;
                    rank_sample.timestamp = (*it).second.timestamp;
                    rank_sample.progress = (*it).second.progress;
                    rank_sample.runtime = 0.0;
                    // Dereference of find result below will
                    // segfault with bad profile sample data
                    size_t rank_idx = (*(m_rank_idx_map.find((*it).second.rank))).second;
                    if ((*it).second.region_id != m_region_id[rank_idx]) {
                        m_rank_sample_prev[rank_idx]->clear();
                    }
                    if (rank_sample.progress == 1.0) {
                        m_region_id[rank_idx] = GEOPM_REGION_ID_UNMARKED;
                    }
                    else {
                        m_region_id[rank_idx] = (*it).second.region_id;
                    }
                    m_rank_sample_prev[rank_idx]->insert(rank_sample);
                }
            }
        }
    }

    void SampleRegulator::insert(std::vector<double>::const_iterator platform_sample_begin,
                                 std::vector<double>::const_iterator platform_sample_end)
    {
        if (!m_aligned_signal.size()) {
            m_num_platform_signal = std::distance(platform_sample_begin, platform_sample_end);
            m_aligned_signal.resize(m_num_platform_signal + M_NUM_RANK_SIGNAL * m_num_rank);
        }
        std::copy(platform_sample_begin, platform_sample_end, m_aligned_signal.begin());
    }

    void SampleRegulator::align(const struct geopm_time_s &timestamp)
    {
        int i = 0;
        double delta;
        double factor;
        double dsdt;
        double progress;
        double runtime;
        geopm_time_s timestamp_prev[2];
        m_aligned_time = timestamp;
        for (auto it = m_rank_sample_prev.begin(); it != m_rank_sample_prev.end(); ++it) {
            switch((*it)->size()) {
                case M_INTERP_TYPE_NONE:
                    // if there is no data, set progress to zero and mark invalid by setting runtime to -1
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i] = 0.0;
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i + 1] = -1.0;
                    break;
                case M_INTERP_TYPE_NEAREST:
                    // if there is only one sample insert it directly
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i] = (*it)->value(0).progress;
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i + 1] = (*it)->value(0).runtime; /// @todo runtime is not set by application, this is uninitialized memory
                    break;
                case M_INTERP_TYPE_LINEAR:
                    // if there are two samples, extrapolate to the given timestamp
                    timestamp_prev[0] = (*it)->value(0).timestamp;
                    timestamp_prev[1] = (*it)->value(1).timestamp;
                    delta = geopm_time_diff(timestamp_prev + 1, &timestamp);
                    factor = 1.0 / geopm_time_diff(timestamp_prev, timestamp_prev + 1);
                    dsdt = ((*it)->value(1).progress - (*it)->value(0).progress) * factor;
                    dsdt = dsdt > 0.0 ? dsdt : 0.0; // progress and runtime are monotonically increasing
                    if ((*it)->value(1).progress == 1.0) {
                        progress = 1.0;
                    }
                    else if ((*it)->value(0).progress == 0.0) {
                        progress = 0.0;
                    }
                    else {
                        progress = (*it)->value(1).progress + dsdt * delta;
                        progress = progress >= 0.0 ? progress : 1e-9;
                        progress = progress <= 1.0 ? progress : 1 - 1e-9;
                    }
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i] = progress;
                    dsdt = ((*it)->value(1).runtime - (*it)->value(0).runtime) * factor;
                    runtime = (*it)->value(1).runtime + dsdt * delta; /// @todo runtime is not set by application, this is uninitialized memory
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i + 1] = runtime;
                    break;
                default:
                    throw Exception("SampleRegulator::align_prof() CircularBuffer has more than two values", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                    break;
            }
            ++i;
        }
    }

}

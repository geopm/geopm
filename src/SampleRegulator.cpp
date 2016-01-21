/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <string.h>
#include "SampleRegulator.hpp"

namespace geopm
{
    SampleRegulator::SampleRegulator(const std::vector<int> &cpu_rank)
        : m_region_id_prev(0)
    {
        std::set<int> rank_set;
        for (auto it = cpu_rank.begin(); it != cpu_rank.end(); ++it) {
            rank_set.insert(*it);
        }
        m_num_rank = rank_set.size();
        int i = 0;
        for (auto it = rank_set.begin(); it != rank_set.end(); ++it) {
            m_rank_idx_map.insert(std::pair<int, int>(*it, i));
            ++i;
        }
        for (int i = 0; i < m_num_rank; ++i) {
            m_rank_sample_prev.emplace_back(M_INTERP_TYPE_LINEAR); // two samples are required for linear interpolation
        }
    }

    SampleRegulator::~SampleRegulator()
    {

    }

    void SampleRegulator::operator () (const struct geopm_time_s &platform_sample_time,
                                       const std::vector<double> &signal_domain_matrix,
                                       std::vector<double>::const_iterator platform_sample_begin,
                                       std::vector<double>::const_iterator platform_sample_end,
                                       std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                       std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end,
                                       std::stack<struct geopm_telemetry_message_s> &telemetry) // result list per domain of control
    {
        // Insert new application profile data into buffers
        insert(prof_sample_begin, prof_sample_end);

        // Populate class member with input platform data
        insert(platform_sample_begin, platform_sample_end);

        // Extrapolate application profile data to time of platform telemetry sample
        align(platform_sample_time);

        // Transform from signal domain to control domain
        transform(signal_domain_matrix, telemetry);
    }

    void SampleRegulator::insert(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                 std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end)
    {
        if (prof_sample_begin != prof_sample_end) {
            bool is_sync = true;
            uint64_t region_id = (*(prof_sample_end - 1)).second.region_id;
            int rank_curr = (*(prof_sample_end - 1)).second.rank;
            for (auto it = prof_sample_end - 1; is_sync && it != prof_sample_begin - 1; --it) {
                if ((*it).second.rank != rank_curr) {
                    if ((*it).second.region_id != region_id ||
                        (*it).second.progress == 1.0) {
                        is_sync = false;
                    }
                    rank_curr = (*it).second.rank;
                }
            }
            if (!is_sync || region_id != m_region_id_prev) {
                for (auto it = m_rank_sample_prev.begin(); it != m_rank_sample_prev.end(); ++it) {
                    (*it).clear();
                }
                m_region_id_prev = 0;
            }
            if (is_sync) {
                struct m_rank_sample_s rank_sample;
                for (auto it = prof_sample_begin; it != prof_sample_end; ++it) {
                    if ((*it).second.region_id == region_id) {
                        rank_sample.timestamp = (*it).second.timestamp;
                        rank_sample.progress = (*it).second.progress;
                        /// @todo: FIXME geopm_prof_message_s does not contain runtime
                        // rank_sample.runtime = (*it).second.runtime;
                        rank_sample.runtime = 0.0;
                        // Dereference of find result below will
                        // segfault with bad profile sample data
                        size_t rank_idx = (*(m_rank_idx_map.find((*it).second.rank))).second;
                        m_rank_sample_prev[rank_idx].insert(rank_sample);
                    }
                }
                m_region_id_prev = region_id;
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
            switch((*it).size()) {
                case M_INTERP_TYPE_NONE:
                    // if there is no data, just fill with zeros
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i] = 0.0;
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i + 1] = 0.0;
                    break;
                case M_INTERP_TYPE_NEAREST:
                    // if there is only one sample insert it directly
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i] = (*it).value(0).progress;
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i + 1] = (*it).value(0).runtime;
                    break;
                case M_INTERP_TYPE_LINEAR:
                    // if there are two samples, extrapolate to the given timestamp
                    timestamp_prev[0] = (*it).value(0).timestamp;
                    timestamp_prev[1] = (*it).value(1).timestamp;
                    delta = geopm_time_diff(timestamp_prev + 1, &timestamp);
                    factor = 1.0 / geopm_time_diff(timestamp_prev, timestamp_prev + 1);
                    dsdt = ((*it).value(1).progress - (*it).value(0).progress) * factor;
                    dsdt = dsdt >= 0.0 ? dsdt : 0.0; // progress and runtime are monotonically increasing
                    progress = (*it).value(1).progress + dsdt * delta;
                    progress = progress >= 0.0 ? progress : 0.0;
                    progress = progress <= 1.0 ? progress : 1.0;
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i] = progress;
                    dsdt = ((*it).value(1).runtime - (*it).value(0).runtime) * factor;
                    runtime = (*it).value(1).runtime + dsdt * delta;
                    m_aligned_signal[m_num_platform_signal + M_NUM_RANK_SIGNAL * i + 1] = runtime;
                    break;
                default:
                    throw Exception("SampleRegulator::align_prof() CircularBuffer has more than two values", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                    break;
            }
            ++i;
        }
    }

    void SampleRegulator::transform(const std::vector<double> &signal_domain_matrix,
                                    std::stack<struct geopm_telemetry_message_s> &telemetry)
    {
        struct geopm_telemetry_message_s tmp_telemetry = {m_region_id_prev, m_aligned_time, {0}};
        size_t num_signal = m_aligned_signal.size();
        size_t num_domain_signal = signal_domain_matrix.size() / num_signal;
        size_t num_domain = num_domain_signal / GEOPM_NUM_SIGNAL_TYPE;
        std::vector<double> result(num_domain_signal);

        std::fill(result.begin(), result.end(), 0.0);
        for (unsigned i = 0; i < num_domain_signal; ++i) {
            for (unsigned j = 0; j < num_signal; ++j) {
                result[i] += signal_domain_matrix[i * num_signal + j] * m_aligned_signal[j];
            }
        }

        // Insert backwards so poping the stack moves forward
        for (int i = num_domain - 1; i != -1; --i) {
            memcpy(tmp_telemetry.signal,
                   result.data() + i * GEOPM_NUM_SIGNAL_TYPE,
                   GEOPM_NUM_SIGNAL_TYPE * sizeof(double));
            telemetry.push(tmp_telemetry);
        }
    }
}

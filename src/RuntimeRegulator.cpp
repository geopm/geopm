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

#include "Exception.hpp"
#include "RuntimeRegulator.hpp"

#include "config.h"

namespace geopm
{
    RuntimeRegulator::RuntimeRegulator(int max_rank_count)
        : M_TIME_ZERO ((struct geopm_time_s){{0, 0}})
        , m_max_rank_count (max_rank_count)
        , m_last_avg (0.0)
        , m_runtimes(m_max_rank_count, std::make_pair(M_TIME_ZERO, 0.0))
    {
        if (m_max_rank_count <= 0) {
            throw Exception("RuntimeRegulator::RuntimeRegulator(): invalid max rank count", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    RuntimeRegulator::~RuntimeRegulator()
    {
    }

    void RuntimeRegulator::record_entry(int rank, struct geopm_time_s entry_time)
    {
        if (rank < 0 || rank >= m_max_rank_count) {
            throw Exception("RuntimeRegulator::record_entry(): invalid rank value", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
#ifdef GEOPM_DEBUG
        if (geopm_time_diff(&m_runtimes[rank].first, &M_TIME_ZERO) != 0.0) {
            throw Exception("RuntimeRegulator::record_entry(): rank re-entry before exit detected", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_runtimes[rank].first = entry_time;
    }

    void RuntimeRegulator::record_exit(int rank, struct geopm_time_s exit_time)
    {
        if (rank < 0 || rank >= m_max_rank_count) {
            throw Exception("RuntimeRegulator::record_exit(): invalid rank value", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double delta = geopm_time_diff(&m_runtimes[rank].first, &exit_time);
        m_runtimes[rank].second = delta;
        m_runtimes[rank].first = M_TIME_ZERO; // record exit
        update_average();
    }

    void RuntimeRegulator::update_average(void)
    {
        double sum = 0.0;
        int num_vals = 0;
        for (auto &val : m_runtimes) {
            sum += val.second;
            if (val.second != 0.0) {
                ++num_vals;
            }
        }

        if (num_vals) {
            m_last_avg = sum / num_vals;
        }
    }

    void RuntimeRegulator::insert_runtime_signal(std::vector<struct geopm_telemetry_message_s> &telemetry)
    {
        /// @todo proper domain averaging
        for (auto &domain_tel : telemetry) {
            domain_tel.signal[GEOPM_TELEMETRY_TYPE_RUNTIME] = m_last_avg;
        }
    }

    std::vector<double> RuntimeRegulator::runtimes(void) const
    {
        std::vector<double> result(m_runtimes.size());
        for (size_t rr = 0; rr < m_runtimes.size(); ++rr) {
            result[rr] = m_runtimes[rr].second;
        }
        return result;
    }

    MPIRuntimeRegulator::MPIRuntimeRegulator(int max_rank_count)
        : RuntimeRegulator(max_rank_count)
    {
    }

    void MPIRuntimeRegulator::update_average(void)
    {
        double sum = 0.0;
        for (auto &val : m_runtimes) {
            sum += val.second;
        }

        m_last_avg = sum / m_runtimes.size();
    }

}

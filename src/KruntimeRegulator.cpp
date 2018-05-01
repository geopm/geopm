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
#include "KruntimeRegulator.hpp"

#include "config.h"

namespace geopm
{

    const geopm_time_s IKruntimeRegulator::M_TIME_ZERO = {{0,0}};

    KruntimeRegulator::KruntimeRegulator(int num_rank)
        : m_num_rank(num_rank)
        , m_rank_log(m_num_rank, m_log_s {M_TIME_ZERO, 0.0, 0.0, 0})
    {
        if (m_num_rank <= 0) {
            throw Exception("KruntimeRegulator::KruntimeRegulator(): invalid max rank count",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void KruntimeRegulator::record_entry(int rank, struct geopm_time_s enter_time)
    {
        if (rank < 0 || rank >= m_num_rank) {
            throw Exception("KruntimeRegulator::record_entry(): invalid rank value",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (geopm_time_diff(&m_rank_log[rank].enter_time, &M_TIME_ZERO) != 0.0) {
            throw Exception("KruntimeRegulator::record_entry(): rank re-entry before exit detected",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_rank_log[rank].enter_time = enter_time;
    }

    void KruntimeRegulator::record_exit(int rank, struct geopm_time_s exit_time)
    {
        if (rank < 0 || rank >= m_num_rank) {
            throw Exception("KruntimeRegulator::record_exit(): invalid rank value",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (geopm_time_diff(&m_rank_log[rank].enter_time, &M_TIME_ZERO) == 0.0) {
            throw Exception("KruntimeRegulator::record_exit(): exit before entry",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        double delta = geopm_time_diff(&m_rank_log[rank].enter_time, &exit_time);
        m_rank_log[rank].last_runtime = delta;
        m_rank_log[rank].enter_time = M_TIME_ZERO; // record exit
        m_rank_log[rank].total_runtime += delta;
        ++m_rank_log[rank].count;
    }

    std::vector<double> KruntimeRegulator::per_rank_last_runtime(void) const
    {
        std::vector<double> result(m_num_rank);
        for (int rr = 0; rr < m_num_rank; ++rr) {
            result[rr] = m_rank_log[rr].last_runtime;
        }
        return result;
    }

    std::vector<double> KruntimeRegulator::per_rank_total_runtime(void) const
    {
        std::vector<double> result(m_num_rank);
        for (int rr = 0; rr < m_num_rank; ++rr) {
            result[rr] = m_rank_log[rr].total_runtime;
        }
        return result;
    }

    std::vector<double> KruntimeRegulator::per_rank_count(void) const
    {
        std::vector<double> result(m_num_rank);
        for (int rr = 0; rr < m_num_rank; ++rr) {
            result[rr] = m_rank_log[rr].count;
        }
        return result;
    }
}

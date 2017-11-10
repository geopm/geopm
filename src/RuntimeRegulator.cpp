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

#include <vector>
#include <string>
#include "geopm_time.h"
#include "geopm_message.h"
#include "Exception.hpp"
#include "RuntimeRegulator.hpp"

namespace geopm
{
    RuntimeRegulator::RuntimeRegulator()
        : m_max_rank_count (0)
        , m_num_entered (0)
        , m_last_avg (0.0)
    {
    }

    RuntimeRegulator::RuntimeRegulator(int max_rank_count)
        : m_max_rank_count (max_rank_count)
        , m_num_entered (0)
        , m_last_avg (0.0)
    {
        if (m_max_rank_count <= 0) {
            throw Exception("RuntimeRegulator::RuntimeRegulator(): invalid max rank count", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_runtimes.resize(m_max_rank_count, std::make_pair((struct geopm_time_s){{0, 0}}, 0.0));
    }

    RuntimeRegulator::~RuntimeRegulator()
    {
    }

    void RuntimeRegulator::record_entry(int rank, struct geopm_time_s entry_time)
    {
        if (rank < 0 || rank >= m_max_rank_count) {
            throw Exception("RuntimeRegulator::record_entry(): invalid rank value", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_runtimes[rank].first = entry_time;
        ++m_num_entered;
    }

    void RuntimeRegulator::record_exit(int rank, struct geopm_time_s exit_time)
    {
        if (rank < 0 || rank >= m_max_rank_count) {
            throw Exception("RuntimeRegulator::record_exit(): invalid rank value", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double runtime = geopm_time_diff(&m_runtimes[rank].first, &exit_time);
        m_runtimes[rank].second = runtime;
        --m_num_entered;
        if (!m_num_entered) { // if everyone has exited lets update the average runtime
            RuntimeRegulator::runtime();
        }
    }

    double RuntimeRegulator::runtime()
    {
        double sum = 0.0;
        int num_vals = 0;
        for (auto &val : m_runtimes) {
            sum += val.second;
            if (val.second != 0.0) {
                ++num_vals;
                if (m_num_entered == 0) {
                    // we are recording this ranks time now zero
                    // out to not contaminate next average calculation
                    val.second = 0.0;
                }
            }
        }

        if (num_vals == 0) {
            return m_last_avg;
        }

        m_last_avg = sum / num_vals;
        return m_last_avg;
    }

    void RuntimeRegulator::insert_runtime_signal(std::vector<struct geopm_telemetry_message_s> &telemetry)
    {
        // TODO proper domain averaging
        for (auto &domain_tel : telemetry) {
            domain_tel.signal[GEOPM_TELEMETRY_TYPE_RUNTIME] = runtime();
        }
    }
}

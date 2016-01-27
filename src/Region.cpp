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

#include "Region.hpp"

namespace geopm
{
    Region::Region(uint64_t identifier, int hint, int num_domain)
        : m_identifier(identifier)
        , m_hint(hint)
        , m_num_domain(num_domain)
        , m_telemetry_matrix(m_num_domain * GEOPM_NUM_SIGNAL_TYPE)
        , m_entry_telemetry(m_num_domain)
        , m_domain_sample(m_num_domain)
        , m_is_dirty_domain_sample(m_num_domain)
        , m_curr_sample({m_identifier, 0.0, 0.0, 0.0})
        , m_domain_buffer(M_NUM_SAMPLE_HISTORY)
        , m_time_buffer(M_NUM_SAMPLE_HISTORY)
    {
        std::fill(m_is_dirty_domain_sample.begin(), m_is_dirty_domain_sample.end(), true);
    }

    Region::~Region()
    {

    }

    void Region::insert(std::stack<struct geopm_telemetry_message_s> &telemetry_stack)
    {
        if (telemetry_stack.size()!= m_num_domain) {
            throw Exception("Region::insert(): telemetry stack not properly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_time_buffer.insert(telemetry_stack.top().timestamp);
        unsigned domain_idx;
        size_t offset = 0;
        for (domain_idx = 0; !telemetry_stack.empty(); ++domain_idx) {
            if (telemetry_stack.top().signal[GEOPM_SIGNAL_TYPE_PROGRESS] == 0.0) {
                m_entry_telemetry[domain_idx] = telemetry_stack.top();
            }
            if (telemetry_stack.top().signal[GEOPM_SIGNAL_TYPE_PROGRESS] == 1.0) {
                m_is_dirty_domain_sample[domain_idx] = false;
                m_domain_sample[domain_idx].runtime = geopm_time_diff(&(m_entry_telemetry[domain_idx].timestamp), &(telemetry_stack.top().timestamp));
                m_domain_sample[domain_idx].energy = (telemetry_stack.top().signal[GEOPM_SIGNAL_TYPE_PKG_ENERGY] + 
                                                      telemetry_stack.top().signal[GEOPM_SIGNAL_TYPE_DRAM_ENERGY]) -
                                                     (m_entry_telemetry[domain_idx].signal[GEOPM_SIGNAL_TYPE_PKG_ENERGY] + 
                                                      m_entry_telemetry[domain_idx].signal[GEOPM_SIGNAL_TYPE_DRAM_ENERGY]);
                m_domain_sample[domain_idx].frequency = (telemetry_stack.top().signal[GEOPM_SIGNAL_TYPE_CLK_UNHALTED_CORE] -
                                                         m_entry_telemetry[domain_idx].signal[GEOPM_SIGNAL_TYPE_CLK_UNHALTED_CORE]) /
                                                        m_domain_sample[domain_idx].runtime;
            }
            memcpy(m_telemetry_matrix.data() + offset, telemetry_stack.top().signal, GEOPM_NUM_SIGNAL_TYPE * sizeof(double));
            offset += GEOPM_NUM_SIGNAL_TYPE;
            telemetry_stack.pop();
        }
        m_domain_buffer.insert(m_telemetry_matrix);
        for (domain_idx = 0; !m_is_dirty_domain_sample[domain_idx] && domain_idx != m_num_domain; ++domain_idx);
        if (domain_idx == m_num_domain) {
            m_curr_sample.runtime = 0.0;
            m_curr_sample.energy = 0.0;
            m_curr_sample.frequency = 0.0;
            for (domain_idx = 0; domain_idx != m_num_domain; ++domain_idx) {
                m_curr_sample.runtime = m_domain_sample[domain_idx].runtime > m_curr_sample.runtime ?
                                        m_domain_sample[domain_idx].runtime : m_curr_sample.runtime;
                m_curr_sample.energy += m_domain_sample[domain_idx].energy;
                m_curr_sample.frequency += m_domain_sample[domain_idx].frequency;
                m_is_dirty_domain_sample[domain_idx] = true;
            }
            m_curr_sample.frequency /= m_num_domain;
        }
    }

    uint64_t Region::identifier(void) const
    {
        return m_identifier;
    }

    int Region::hint(void) const
    {
        return m_hint;
    }

    void Region::sample_message(struct geopm_sample_message_s &sample)
    {
        sample = m_curr_sample;
    }

    void Region::statistics(int domain_idx, int signal_type, double result[]) const
    {
        throw Exception("Region::statistics()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double Region::integrate_time(int domain_idx, int signal_type, double &delta_time, double &integral) const
    {
        throw Exception("Region::integrate_time()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return 0.0;
    }
}

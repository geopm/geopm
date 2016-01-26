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

#include "Region.hpp"

namespace geopm
{
    Region::Region(uint64_t identifier, int hint, int num_domain)
        : m_identifier(identifier)
        , m_hint(hint)
        , m_num_domain(num_domain)
        , m_is_current(false)
        , m_telemetry_matrix(m_num_domain * GEOPM_NUM_SIGNAL_TYPE)
        , m_domain_buffer(m_num_domain)
        , m_time_buffer(m_num_domain)
    {

    }

    Region::~Region()
    {

    }

    void Region::insert(std::stack<struct geopm_telemetry_message_s> &telemetry_stack)
    {
        if (telemetry_stack.size()!= m_num_domain) {
            throw Exception("Region::insert(): telemetry stack not properly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        size_t offset = 0;
        m_time_buffer.insert(telemetry_stack.top().timestamp);
        while (!telemetry_stack.empty()) {
            memcpy(m_telemetry_matrix.data() + offset, telemetry_stack.top().signal, GEOPM_NUM_SIGNAL_TYPE * sizeof(double));
            offset += GEOPM_NUM_SIGNAL_TYPE;
            telemetry_stack.pop();
        }
        m_domain_buffer.insert(m_telemetry_matrix);
    }

    uint64_t Region::identifier(void) const
    {
        return m_identifier;
    }

    int Region::hint(void) const
    {
        return m_hint;
    }

    void Region::policy(Policy* policy)
    {
        m_policy = *policy;
    }

    struct geopm_policy_message_s* Region::last_policy(void)
    {
        return &m_last_policy;
    }

    void Region::sample_message(struct sample_message_s &sample)
    {
        sample.region_id = m_identifier;
        sample.runtime = ;
        sample.energy = ;
        sample.frequency = ;
    }

    void Region::last_policy(const struct geopm_policy_message_s &policy)
    {
        m_last_policy = policy;
    }

    Policy* Region::policy(void)
    {
        return &m_policy;
    }

    std::vector <struct geopm_policy_message_s>* Region::split_policy(void)
    {
        return &m_split_policy;
    }

    std::vector <struct geopm_sample_message_s>* Region::child_sample(void)
    {
        return &m_child_sample;
    }

    double Region::observation_mean(int buffer_index) const
    {
        return m_obs.mean(buffer_index);
    }

    double Region::observation_median(int buffer_index) const
    {
        return m_obs.median(buffer_index);
    }

    double Region::observation_stddev(int buffer_index) const
    {
        return m_obs.stddev(buffer_index);
    }

    double Region::observation_max(int buffer_index) const
    {
        return m_obs.max(buffer_index);
    }

    double Region::observation_min(int buffer_index) const
    {
        return m_obs.min(buffer_index);
    }

    double Region::observation_integrate_time(int buffer_index) const
    {
        return m_obs.integrate_time(buffer_index);
    }
}
